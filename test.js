var assert = require('assert');
var bundles = require('./build/Release/BundlesAddon.node');

function randomIntInc (low, high) {
    return Math.floor(Math.random() * (high - low + 1) + low);
}
console.log("Creating bundle");

var bundle = new bundles.Bundle("someBundle.dat", ["Read", "Write", "OpenAlways"]);

console.log("Testing bundle attributes");

var attrOne;
var attrTwo;
var attrThree;

var fileOne;
var fileTwo;
var fileThree;

var block = new Buffer(1024 * 1024);

var totalWriteBlocks = 10;
var totalReadBlocks = 10;

var pAttrSet = function () {
    return new Promise(
    function (resolve, reject) {
        // test attributes
        bundle.AttributeSet("Private", "Some Private Data", function (error) {
            assert(error === undefined);
            bundle.AttributeSet("Public", Buffer.from("Some Public Data"), function (error) {
                assert(error === undefined);
                bundle.AttributeSet("System", "Some System Data", function (error) {
                    assert(error === undefined);
                    resolve();
                });
            });
        });
    });
}

var pAttrGet = function () {
    return new Promise(
    function (resolve, reject) {
        bundle.AttributeGet("Private", function (error, data) {
            assert(error === undefined);
            attrOne = data;
            bundle.AttributeGet("Public", function (error, data) {
                assert(error === undefined);
                attrTwo = data;
                bundle.AttributeGet("System", function (error, data) {
                    assert(error === undefined);
                    attrThree = data;
                    resolve();
                });
            });
        });
    });
}

var pFileAttrSet = function () {
    return new Promise(
    function (resolve, reject) {
        // test attributes
        bundle.FileAttributeSet(fileOne, "Some Data One", function (error) {
            assert(error === undefined);
            bundle.FileAttributeSet(fileTwo, Buffer.from("Some Data Two"), function (error) {
                assert(error === undefined);
                bundle.FileAttributeSet(fileThree, "Some Data Three", function (error) {
                    assert(error === undefined);
                    resolve();
                });
            });
        });
    });
}

var pFileAttrGet = function () {
    return new Promise(
    function (resolve, reject) {
        bundle.FileAttributeGet(fileOne, function (error, data) {
            assert(error === undefined);
            attrOne = data;
            bundle.FileAttributeGet(fileTwo, function (error, data) {
                assert(error === undefined);
                attrTwo = data;
                bundle.FileAttributeGet(fileThree, function (error, data) {
                    assert(error === undefined);
                    attrThree = data;
                    resolve();
                });
            });
        });
    });
}

var pFileWrite = function (fileIdx) {
    return new Promise(
    function (resolve, reject) {
        bundle.FileWrite(fileIdx, block, function (error) {
            assert(error === undefined);
            totalWriteBlocks--;
            if (totalWriteBlocks == 0) {
                resolve;
            }
        });
    });
}

var pFileRead = function (fileIdx) {
    return new Promise(
    function (resolve, reject) {
        bundle.FileRead(fileIdx, block.length, function (error, data) {
            assert(data.length == block.length);
            assert(data.compare(block) == 0);

            totalReadBlocks--;
            if (totalReadBlocks == 0) {
                resolve;
            }
        });
    });
}

pAttrSet().then(function () {
    return pAttrGet();
}).then(function () {
    assert(attrOne == "Some Private Data");
    assert(attrTwo == "Some Public Data");
    assert(attrThree == "Some System Data");
    console.log("Done!");

    // test files
    console.log("Testing files");
    fileOne = bundle.FileOpen("FileOne.dat", true);
    fileTwo = bundle.FileOpen("FileTwo.dat", true);
    fileThree = bundle.FileOpen("FileThree.dat", true);

    assert(fileOne > 0);
    assert(fileTwo > 0);
    assert(fileThree > 0);
    console.log("Done!");

    console.log("Testing files attributes");
    return pFileAttrSet();
}).then(function () {
    return pFileAttrGet();
}).then(function () {
    assert(attrOne == "Some Data One");
    assert(attrTwo == "Some Data Two");
    assert(attrThree == "Some Data Three");

    console.log("Testing file read/write");

    for (var i = 0; i < block.length; i++) {
        block[i] = randomIntInc(0, 255)
    }

    for (var i = 0; i < totalWriteBlocks; i++) {
        pFileWrite(fileOne);
    }
    return;
}).then(function () {
    assert(bundle.FileSeek(fileOne, 0, "Set") == 0);

    for (var i = 0; i < totalReadBlocks; i++) {
        pFileRead(fileOne);
    }

    return;
}).then(function () {

    assert(bundle.FileOpen("FileOne.dat", false) == fileOne);
    bundle.FileDelete(fileOne);

    var bundleFiles = bundle.FileNames();
    assert(bundleFiles.length == 2);

    assert(bundle.FileOpen("FileOne.dat", false) == -1);

    console.log("Done!");

}).catch(function (e) {
    console.log(e);
});