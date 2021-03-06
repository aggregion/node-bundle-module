const should = require('chai').should();
const AggregionBundle = require('./index');
const path = require('path');
const crypto = require('crypto');
const temp = require('temp');
const fs = require('fs');
const fsExtra = require('fs-extra');
const exec = require('child_process').exec;


describe('AggregionBundle', () => {

    const testBundlePath = temp.path() + '.agb';
    const bundleInfo = new Buffer(JSON.stringify({
        contentId: 1
    }), 'UTF-8');
    const bundleProperties = new Buffer(JSON.stringify({
        main_file: 'test.dat'
    }), 'UTF-8');

    const createBundle = () => {
        return new AggregionBundle({
            path: testBundlePath
        });
    };

    const fillBundle = (bundle) => {
        return new Promise((resolve, reject) => {
            crypto.randomBytes(256, (err, buff) => {
                if (err) {
                    return reject(err);
                }
                let testFiles = [];
                for (let i = 0; i < 100; i++) {
                    let depth = i % 4;
                    let path = '';
                    for (let j = 0; j < depth; j++) {
                        path += `dir${i}_${j}/`;
                    }
                    path += `file${i}.dat`;
                    let testFile = {
                        name: path,
                        data: buff,
                        props: new Buffer(path, 'UTF-8')
                    };
                    testFiles.push(testFile);
                }
                let testNames = testFiles.map((f) => f.name);
                Promise.resolve()
                    .then(() => {
                        return bundle.setBundleInfoData(bundleInfo);
                    })
                    .then(() => {
                        return bundle.setBundlePropertiesData(bundleProperties);
                    })
                    .then(() => {
                        let filePromises = [];
                        testFiles.forEach((f) => {
                            filePromises.push(Promise.resolve()
                                .then(() => {
                                    return bundle.createFile(f.name);
                                })
                                .then((fd) => {
                                    return bundle
                                        .writeFileBlock(fd, f.data)
                                        .then(() => {
                                            return bundle.writeFilePropertiesData(fd, f.props);
                                        });
                                })
                            );
                        });
                        return Promise.all(filePromises);
                    })
                    .then(() => {
                        resolve({
                            testNames
                        });
                    })
                    .catch(reject);
            });
        });
    };

    describe('#constructor', () => {
        it('should open bundle', () => {
            try {
                let bundle;
                should.not.throw(() => {
                    bundle = createBundle();
                    bundle.close();
                });
            } finally {
                fs.unlinkSync(testBundlePath);
            }
        });

        it('should throw if invalid arguments passed', () => {
            should.throw(() => new AggregionBundle());
        });

    });

    describe('#getFiles', () => {
        it('should return valid list of files', (done) => {
            let bundle = createBundle();
            fillBundle(bundle)
                .then(({testNames}) => {
                    return bundle.getFiles()
                        .then((bundleFiles) => {
                            bundleFiles.should.include.members(testNames);
                            testNames.should.include.members(bundleFiles);
                            bundle.close();
                            done();
                        });
                })
                .catch(done);
        });
    });

    describe('#getBundleInfoData', () => {
        it('should return valid info', (done) => {
            let bundle = createBundle();
            bundle
                .getBundleInfoData()
                .then((data) => {
                    bundleInfo.compare(data).should.equal(0);
                    bundle.close();
                    done();
                });
        });
    });

    describe('#setBundleInfoData', () => {
        it('should set bundle info', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            bundle
                .setBundleInfoData(bundleInfo)
                .then(() => {
                    bundle.close();
                    let bundle2 = new AggregionBundle({
                        path: tempPath
                    });
                    return bundle2
                        .getBundleInfoData()
                        .then((data) => {
                            bundleInfo.compare(data).should.equal(0);
                            bundle2.close();
                        });
                })
                .catch(done)
                .then(() => {
                    fs.unlinkSync(tempPath);
                    done();
                });
        });
    });

    describe('#getBundlePropertiesData', () => {
        it('should return valid properties', (done) => {
            let bundle = createBundle();
            bundle
                .getBundlePropertiesData()
                .then((data) => {
                    bundleProperties.compare(data).should.equal(0);
                    bundle.close();
                    fs.unlinkSync(testBundlePath);
                    done();
                });
        });
    });

    describe('#setBundlePropertiesData', () => {
        it('should set bundle properties', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            bundle
                .setBundlePropertiesData(bundleProperties)
                .then(() => {
                    bundle.close();
                    let bundle2 = new AggregionBundle({
                        path: tempPath
                    });
                    return bundle2
                        .getBundlePropertiesData()
                        .then((data) => {
                            bundleProperties.compare(data).should.equal(0);
                            bundle2.close();
                        });
                })
                .catch(done)
                .then(() => {
                    fs.unlinkSync(tempPath);
                    done();
                });
        });
    });

    describe('#createFile', () => {
        it('should create file in the new bundle', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            const filePath = 'dir1/dir2/file.dat';
            bundle
                .createFile(filePath)
                .then(() => {
                    bundle.close();
                    let bundle2 = new AggregionBundle({
                        path: tempPath
                    });
                    return bundle2
                        .getFiles()
                        .then((files) => {
                            files.should.have.lengthOf(1);
                            files[0].should.equal(filePath);
                            bundle2.close();
                        });
                })
                .catch(done)
                .then(() => {
                    fs.unlinkSync(tempPath);
                    done();
                });
        });

        it('should append file to the existing bundle', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            fillBundle(bundle)
                .then(() => {
                    bundle.close();
                    return new AggregionBundle({
                        path: tempPath
                    });
                })
                .then((bundle) => {
                    const filePath = 'dir1/dir2/file.dat';
                    return bundle
                        .createFile(filePath)
                        .then(() => {
                            bundle.close();
                            let bundle2 = new AggregionBundle({
                                path: tempPath
                            });
                            return bundle2
                                .getFiles()
                                .then((files) => {
                                    files.should.have.lengthOf(101);
                                    bundle2.close();
                                });
                        });
                })
                .catch((e) => {
                    done(e);
                })
                .then(() => {
                    fs.unlinkSync(tempPath);
                    done();
                });
        });
    });

    describe('#openFile', () => {
        it('should open all files in the bundle', (done) => {
            let bundle = createBundle();
            fillBundle(bundle)
                .then(({testNames}) => {
                    let fds = [];
                    testNames.forEach((path, i) => {
                        let fd = bundle.openFile(path);
                        fd.should.not.be.undefined;
                        fds.should.not.include(fd);
                        fds.push(fd);
                    });
                    bundle.close();
                    fs.unlinkSync(testBundlePath);
                    done();
                })
                .catch(done);
        });
    });

    describe('#deleteFile', () => {
        it('should delete file', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            fillBundle(bundle)
                .then(() => {
                    let filesCount;
                    return bundle
                        .getFiles()
                        .then((files) => {
                            filesCount = files.length;
                            bundle.deleteFile(files[0]);
                            return bundle.getFiles();
                        })
                        .then((files) => {
                            files.length.should.equal(filesCount - 1);
                            bundle.close();
                        })
                        .catch(done)
                        .then(() => {
                            fs.unlinkSync(tempPath);
                            done();
                        });
                })
                .catch(done);
        });
    });

    describe('#getFileSize', () => {
        it('should return valid file size', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            const filePath = 'dir1/dir2/file.dat';
            const data = new Buffer('testString', 'UTF-8');
            bundle
                .createFile(filePath)
                .then((fd) => {
                    return bundle.writeFileBlock(fd, data);
                })
                .then(() => {
                    bundle.getFileSize(filePath).should.equal(data.length);
                    bundle.close();
                })
                .catch(done)
                .then(() => {
                    fs.unlinkSync(tempPath);
                    done();
                });
        });
    });

    describe('#seekFile', () => {
        it('should seek file and return valid block', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            const filePath = 'dir1/dir2/file.dat';
            const data = new Buffer('1234567890', 'ascii');
            const expectedData = new Buffer('67890', 'ascii');
            bundle
                .createFile(filePath)
                .then((fd) => {
                    return bundle.writeFileBlock(fd, data);
                })
                .then(() => {
                    let fd2 = bundle.openFile(filePath);
                    bundle.seekFile(fd2, 5);
                    return bundle.readFileBlock(fd2, expectedData.length);
                })
                .then((readData) => {
                    expectedData.compare(readData).should.equal(0);
                    bundle.close();
                })
                .catch(done)
                .then(() => {
                    fs.unlinkSync(tempPath);
                    done();
                });
        });
    });

    describe('#readFileBlock', () => {
        it('should read file block with real size', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            const filePath = 'dir1/dir2/file.dat';
            const data = new Buffer('1234567890', 'ascii');
            bundle
                .createFile(filePath)
                .then((fd) => {
                    return bundle.writeFileBlock(fd, data);
                })
                .then(() => {
                    let fd2 = bundle.openFile(filePath);
                    const bufSize = 1024;
                    return bundle.readFileBlock(fd2, bufSize);
                })
                .then((readData) => {
                    data.compare(readData).should.equal(0);
                    bundle.close();
                })
                .catch(done)
                .then(() => {
                    fs.unlinkSync(tempPath);
                    done();
                });
        });
    });

    describe('#readFilePropertiesData', () => {
        it('should read file properties', (done) => {
            let bundle = createBundle();
            fillBundle(bundle)
                .then(() => {
                    let testProps;
                    return bundle
                        .getFiles()
                        .then((files) => {
                            let file = files[0];
                            testProps = new Buffer(file, 'UTF-8');
                            let fd = bundle.openFile(file);
                            return bundle.readFilePropertiesData(fd);
                        })
                        .then((readProps) => {
                            testProps.compare(readProps).should.equal(0);
                            bundle.close();
                            fs.unlinkSync(testBundlePath);
                            done();
                        });
                })
                .catch(done);
        });
    });

    describe('#writeFile', () => {
        it('should write block that then will be readable and equal to wrote', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            const filePath = 'dir1/dir2/file.dat';
            const data = new Buffer('testString', 'UTF-8');
            bundle
                .createFile(filePath)
                .then((fd) => {
                    return bundle.writeFileBlock(fd, data);
                })
                .then(() => {
                    let fd2 = bundle.openFile(filePath);
                    return bundle.readFileBlock(fd2, data.length);
                })
                .then((readData) => {
                    data.compare(readData).should.equal(0);
                    bundle.close();
                })
                .catch(done)
                .then(() => {
                    fs.unlinkSync(tempPath);
                    done();
                });
        });
    });

    describe('#writeFilePropertiesData', () => {
        it('should write properties that then will be readable and equal to wrote', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            const filePath = 'dir1/dir2/file.dat';
            const data = new Buffer('testString', 'utf8');
            let bundle2;
            bundle
                .createFile(filePath)
                .then((fd) => {
                    return bundle.writeFilePropertiesData(fd, data);
                })
                .then(() => {
                    bundle.close();
                    bundle2 = new AggregionBundle({
                        path: tempPath,
                        readonly: true
                    });
                    let fd2 = bundle2.openFile(filePath);
                    return bundle2.readFilePropertiesData(fd2);
                })
                .then((readData) => {
                    data.compare(readData).should.equal(0);
                    bundle2.close();
                })
                .catch(done)
                .then(() => {
                    fs.unlinkSync(tempPath);
                    done();
                });
        });

        it('should really write all data to the disk', (done) => {
            let tempPath = temp.path() + '.agb';
            let bundle = new AggregionBundle({
                path: tempPath
            });
            const filePath = 'dir1/dir2/file.dat';
            const data = new Buffer('testString', 'UTF-8');
            bundle
                .createFile(filePath)
                .then((fd) => {
                    return bundle.writeFilePropertiesData(fd, data);
                })
                .then(() => {
                    bundle.close();
                    let script = path.join(__dirname, './utils/readbundle.js');
                    return new Promise((resolve, reject) => {
                        let run = `node ${script} fileprops -p ${filePath} ${tempPath}`;
                        exec(run, (err, stdout, stderr) => {
                            fs.unlinkSync(tempPath);
                            if (err) {
                                return reject(err);
                            }
                            let hex = stdout.replace('\n', '');
                            hex.should.not.be.empty;
                            try {
                                let buf = new Buffer(hex, 'hex');
                                resolve(buf);
                            } catch (e) {
                                reject(e);
                            }
                        });
                    });
                })
                .then((readData) => {
                    data.compare(readData).should.equal(0);
                    done();
                })
                .catch(done);
        });
    });
});