# Module for working with Aggregion Binary bundles

## Installation

`npm install git+ssh://git@stash.aggregion.com:7999/bck/node-bundle-module.git --save`

## Usage

```javascript
let bundle = new AggregionBundle({path: '/path/to/bundle'});

// Get list of files

bundle
    .getFiles()
    .then((fileNames) => {
        console.log(fileNames);
    });

// Get bundle info

bundle
    .getBundleInfoData()
    .then((data) => {
        console.log('info', data);
    });

// Get bundle properties

bundle
    .getBundlePropertiesData()
    .then((data) => {
        console.log('properties', data);
    });

// Set bundle info

bundle
    .setBundleInfoData('some info string')
    .then(() => {
        return bundle.setBundleInfoData(new Buffer('or you can use buffer'));
    })
    .then(() => {
        console.log('done');
    });

// Set bundle properties

bundle
    .setBundlePropertiesData('some info string')
    .then(() => {
        return bundle.setBundlePropertiesData(new Buffer('or you can use buffer'));
    })
    .then(() => {
        console.log('done');
    });

// Create new file

bundle
    .createFile('path/to/file/to/create.dat')
    .then((fd) => {
        console.log(`created file with descriptor: ${fd}`);
    });

// Open existing file

bundle
    .openFile('path/to/existing/file.dat')
    .then((fd) => {
        console.log(`opened file with descriptor: ${fd}`);
    });

// Read file

bundle
    .readFileBlock(bundle.openFile('path/to/existing/file.dat'), 1024 * 1024)
    .then((data) => {
        console.log(`Read block with size: ${data.length}`);
    });

// Read file properties

bundle
    .readFilePropertiesData(bundle.openFile('path/to/existing/file.dat'))
    .then((propsData) => {
        console.log(propsData);
    });

// Seek

let fd = bundle.openFile('path/to/existing/file.dat');


bundle.seekFile(
    fd, 
    1000 // Position from begin
);

// Write file

bundle
    .createFile('path/to/file/to/create.dat')
    .then((fd) => {
        return bundle.writeFileBlock(fd, new Buffer(1000))
    })
    .then(() => {
        console.log('Block written');
    });

// Write file properties

bundle
    .writeFilePropertiesData(bundle.openFile('path/to/existing/file.dat'), 'some props')
    .then(() => {
        console.log('Properties written');
    });

// Get file size

console.log(`Size of file: ${bundle.getFileSize('path/to/existing/file.dat')}`);

// Delete file

bundle.deleteFile('path/to/existing/file.dat');

```

## Test

`npm test`