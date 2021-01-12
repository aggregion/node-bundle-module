/**
 * Types of bundle attributes
 * @enum {string}
 */
declare enum BundleAttributeType {
	SYSTEM = 'System',
	PUBLIC = 'Public',
	PRIVATE = 'Private',
}

declare interface Options {
	path: string;
	readonly?: boolean;
}

/**
 *
 */
declare interface AggregionBundle {

	/**
	 *
	 * @param options
	 */
	new (options: Options);

	/**
	 * Returns list of files in the bundle
	 * @return {Promise.<string[]>}
	 * @return
	 */
	getFiles(): Promise<string[]>;

	/**
	 * Returns bundle info data
	 * @return {Promise.<Buffer>}
	 */
	getBundleInfoData(): Promise<Buffer>;

	/**
	 * Sets bundle info data
	 * @param {Buffer|string} data
	 * @return {Promise}
	 * @param data
	 * @return
	 */
	setBundleInfoData(data : Buffer | string): Promise<void>;

	/**
	 * Returns bundle properties data
	 * @return {Promise.<Buffer>}
	 */
	getBundlePropertiesData(): Promise<Buffer>;

	/**
	 * Sets bundle properties data
	 * @param {Buffer|string} data
	 * @return {Promise}
	 * @param data
	 * @return
	 */
	setBundlePropertiesData(data : Buffer | string): Promise<void>;

	/**
	 * Creates a new file
	 * @param {string} path Path to the file in the bundle
	 * @return {Promise.<number>} File descriptor
	 * @param path
	 * @return
	 */
	createFile(path : string): number;

	/**
	 * Opens an existent file
	 * @param {string} path Path to the file in the bundle
	 * @return {number} File descriptor
	 * @param path
	 * @return
	 */
	openFile(path : string): number;

	/**
	 * Deletes the file
	 * @param {string} path Path to the file in the bundle
	 * @param path
	 */
	deleteFile(path : string): void;

	/**
	 * Returns size of the file
	 * @param {string} path Path to the file in the bundle
	 * @return {number}
	 * @param path
	 * @return
	 */
	getFileSize(path : string): number;

	/**
	 * Move to position in the file
	 * @param {number} fd File descriptor
	 * @param {number} position Position in the file to seek (in bytes)
	 * @param fd
	 * @param position
	 */
	seekFile(fd : number, position : number): void;

	/**
	 * Reads a block of data from the file
	 * @param {number} fd File descriptor
	 * @param {number} size Size of block to read
	 * @return {Promise.<Buffer>} Block data
	 * @param fd
	 * @param size
	 */
	readFileBlock(fd : number, size : number): void;

	/**
	 * Reads attributes data from file
	 * @param {number} fd File descriptor
	 * @return {Promise.<Buffer>} Attributes data
	 * @param fd
	 */
	readFilePropertiesData(fd : number): void;

	/**
	 * Writes block of data to the file
	 * @param {number} fd File descriptor
	 * @param {Buffer|string} data Data to write
	 * @return {Promise}
	 * @param fd
	 * @param data
	 * @return
	 */
	writeFileBlock(fd : number, data : Buffer | string): Promise<void>;

	/**
	 * Writes file attributes
	 * @param {number} fd File descriptor
	 * @param {Buffer|string} data Data to write
	 * @return {Promise}
	 * @param fd
	 * @param data
	 * @return
	 */
	writeFilePropertiesData(fd : number, data : Buffer | string): Promise<void>;

	/**
	 * Closes the bundle
	 */
	close(): void;

	/**
	 * Opens file for read or write
	 * @param {string} path Path to file in the bundle
	 * @param {boolean} [create=false] If "true", then file will be created
	 * @return {number} File descriptor
	 * @private
	 * @param path
	 * @param create?
	 * @return
	 */
	_openFile(path : string, create? : boolean): number;

	/**
	 * Writes attribute to bundle
	 * @param {BundleAttributeType} type Type of attribute
	 * @param {Buffer|string} data Data to write
	 * @return {Promise}
	 * @private
	 * @throws {Error} Will throw if invalid arguments passed
	 * @param type
	 * @param data
	 * @return
	 */
	_writeBundleAttribute(type : BundleAttributeType, data : Buffer | string): Promise<void>;

	/**
	 * Reads attribute from bundle
	 * @param {BundleAttributeType} type
	 * @return {Promise.<Buffer>}
	 * @private
	 * @param type
	 */
	_readBundleAttribute(type : BundleAttributeType): Promise<Buffer>;

	/**
	 * Checks that bundle is not closed
	 * @private
	 */
	_checkNotClosed(): void;
}

declare module '@aggregion/agg-bundle' {
	const AggregionBundle:AggregionBundle;
	export default AggregionBundle;
}
