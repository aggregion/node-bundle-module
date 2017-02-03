const Addon = require('./build/Release/BundlesAddon.node');
const fs = require('fs');
const check = require('check-types');
const Q = require('q');

/**
 * Types of bundle attributes
 * @enum {string}
 */
const BundleAttributeType = {
    SYSTEM: 'System',
    PUBLIC: 'Public',
    PRIVATE: 'Private'
};

class AggregionBundle  {

    /**
     * Constructs a new instance
     * @param {object} options
     * @param {string} options.path Path to file
     */
    constructor(options) {
        check.assert.assigned(options, '"options" is required argument');
        check.assert.assigned(options.path, '"options.path" is required argument');
        check.assert.nonEmptyString(options.path, '"options.path" should be non-empty string');
        let {path} = options;
        this._bundle = new Addon.Bundle(path, ['Read', 'Write', 'OpenAlways']);
    }

    /**
     * Returns list of files in the bundle
     * @return {Promise.<string[]>}
     */
    getFiles() {
        let {_bundle: bundle} = this;
        return Promise.resolve(bundle.FileNames());
    }

    /**
     * Returns bundle info data
     * @return {Promise.<Buffer>}
     */
    getBundleInfoData() {
        return this._readBundleAttribute(BundleAttributeType.PUBLIC);
    }

    /**
     * Sets bundle info data
     * @param {Buffer|string} data
     * @return {Promise}
     */
    setBundleInfoData(data) {
        check.assert.assigned(data, '"data" is required argument');
        return this._writeBundleAttribute(BundleAttributeType.PUBLIC, data);
    }

    /**
     * Returns bundle properties data
     * @return {Promise.<Buffer>}
     */
    getBundlePropertiesData() {
        return this._readBundleAttribute(BundleAttributeType.PRIVATE);
    }

    /**
     * Sets bundle properties data
     * @param {Buffer|string} data
     * @return {Promise}
     */
    setBundlePropertiesData(data) {
        check.assert.assigned(data, '"data" is required argument');
        return this._writeBundleAttribute(BundleAttributeType.PRIVATE, data);
    }


    /**
     * Creates a new file
     * @param {string} path Path to the file in the bundle
     * @return {Promise.<number>} File descriptor
     */
    createFile(path) {
        check.assert.assigned(path, '"path" is required argument');
        check.assert.nonEmptyString(path, '"path" should be non-empty string');
        return Promise.resolve(this._openFile(path, true));
    }

    /**
     * Opens an existent file
     * @param {string} path Path to the file in the bundle
     * @return {number} File descriptor
     */
    openFile(path) {
        check.assert.assigned(path, '"path" is required argument');
        check.assert.nonEmptyString(path, '"path" should be non-empty string');
        return this._openFile(path, false);
    }

    /**
     * Deletes the file
     * @param {string} path Path to the file in the bundle
     */
    deleteFile(path) {
        check.assert.assigned(path, '"path" is required argument');
        check.assert.nonEmptyString(path, '"path" should be non-empty string');
        let {_bundle: bundle} = this;
        let d = this.openFile(path);
        bundle.FileDelete(d);
    }

    /**
     * Returns size of the file
     * @param {string} path Path to the file in the bundle
     * @return {number}
     */
    getFileSize(path) {
        check.assert.assigned(path, '"path" is required argument');
        check.assert.nonEmptyString(path, '"path" should be non-empty string');
        let {_bundle: bundle} = this;
        let d = this.openFile(path);
        return bundle.FileLength(d);
    }

    /**
     * Move to position in the file
     * @param {number} fd File descriptor
     * @param {number} position Position in the file to seek (in bytes)
     */
    seekFile(fd, position) {
        check.assert.assigned(fd, '"fd" is required argument');
        check.assert.assigned(position, '"position" is required argument');
        check.assert.integer(position, '"position" should be integer');
        check.assert.greaterOrEqual(position, 0, '"position" should be greater or equal to 0');
        let {_bundle: bundle} = this;
        bundle.FileSeek(fd, position, 'Set');
    }

    /**
     * Reads a block of data from the file
     * @param {number} fd File descriptor
     * @param {number} size Size of block to read
     * @return {Promise.<Buffer>} Block data
     */
    readFileBlock(fd, size) {
        check.assert.assigned(fd, '"fd" is required argument');
        check.assert.assigned(size, '"size" is required argument');
        check.assert.integer(size, '"size" should be integer');
        check.assert.greaterOrEqual(size, 0, '"size" should be greater or equal to 0');
        let {_bundle: bundle} = this;
        let def = Q.defer();
        bundle.FileRead(fd, size, (err, data) => {
            if (err) {
                def.reject(new Error(err));
            } else {
                def.resolve(data);
            }
        });
        return def.promise;
    }

    /**
     * Reads attributes data from file
     * @param {number} fd File descriptor
     * @return {Promise.<Buffer>} Attributes data
     */
    readFilePropertiesData(fd) {
        check.assert.assigned(fd, '"fd" is required argument');
        let {_bundle: bundle} = this;
        let def = Q.defer();
        bundle.FileAttributeGet(fd, (err, data) => {
            if (err) {
                def.reject(new Error(err));
            } else {
                def.resolve(data);
            }
        });
        return def.promise;
    }

    /**
     * Writes block of data to the file
     * @param {number} fd File descriptor
     * @param {Buffer|string} data Data to write
     * @return {Promise}
     */
    writeFileBlock(fd, data) {
        check.assert.assigned(fd, '"fd" is required argument');
        check.assert.assigned(data, '"data" is required argument');
        if (typeof data === 'string') {
            data = new Buffer(data, 'UTF-8');
        }
        if (!(data instanceof Buffer)) {
            throw new Error('"data" should be Buffer or string');
        }
        let {_bundle: bundle} = this;
        let def = Q.defer();
        bundle.FileWrite(fd, data, (err) => {
            if (err) {
                def.reject(new Error(err));
            } else {
                def.resolve();
            }
        });
        return def.promise;
    }

    /**
     * Writes file attributes
     * @param {number} fd File descriptor
     * @param {Buffer|string} data Data to write
     * @return {Promise}
     */
    writeFilePropertiesData(fd, data) {
        check.assert.assigned(fd, '"fd" is required argument');
        check.assert.assigned(data, '"data" is required argument');
        if (typeof data === 'string') {
            data = new Buffer(data, 'UTF-8');
        }
        if (!(data instanceof Buffer)) {
            throw new Error('"data" should be Buffer or string');
        }
        let {_bundle: bundle} = this;
        let def = Q.defer();
        bundle.FileAttributeSet(fd, data, (err) => {
            if (err) {
                def.reject(new Error(err));
            } else {
                def.resolve();
            }
        });
        return def.promise;
    }

    /**
     * Opens file for read or write
     * @param {string} path Path to file in the bundle
     * @param {boolean} [create=false] If "true", then file will be created
     * @return {number} File descriptor
     * @private
     */
    _openFile(path, create = false) {
        check.assert.nonEmptyString(path, '"path" is required and should be non-empty string');
        check.assert.boolean(create, '"create" should be boolean');
        let {_bundle: bundle} = this;
        return bundle.FileOpen(path, create);
    }

    /**
     * Writes attribute to bundle
     * @param {BundleAttributeType} type Type of attribute
     * @param {Buffer|string} data Data to write
     * @return {Promise}
     * @private
     * @throws {Error} Will throw if invalid arguments passed
     */
    _writeBundleAttribute(type, data) {
        check.assert.nonEmptyString(type, '"type" should be non-empty string');
        check.assert.assigned(data, '"data" is required argument');
        let {_bundle: bundle} = this;
        if (typeof data === 'string') {
            data = new Buffer(data, 'UTF-8');
        }
        if (!(data instanceof Buffer)) {
            throw new Error('"data" should be Buffer or string');
        }
        let def = Q.defer();
        bundle.AttributeSet(type, data, (err) => {
            if (err) {
                def.reject(new Error(err));
            } else {
                def.resolve();
            }
        });
        return def.promise;
    }

    /**
     * Reads attribute from bundle
     * @param {BundleAttributeType} type
     * @return {Promise.<Buffer>}
     * @private
     */
    _readBundleAttribute(type) {
        check.assert.nonEmptyString(type, '"type" should be non-empty string');
        let {_bundle: bundle} = this;
        let def = Q.defer();
        bundle.AttributeGet(type, (err, data) => {
            if (err) {
                def.reject(new Error(err));
            } else {
                def.resolve(data);
            }
        });
        return def.promise;
    }
}

module.exports = AggregionBundle;