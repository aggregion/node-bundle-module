var assert = require('assert');
var bundles = require('./build/Release/BundlesAddon.node');

function randomIntInc (low, high) {
    return Math.floor(Math.random() * (high - low + 1) + low);
}
console.log("Creating bundle");

var bundle = new bundles.Bundle("someBundle.dat", ["Read", "Write", "OpenAlways"]);

console.log("Testing bundle attributes");

var pAttrSet = new Promise(
    function (resolve, reject) {
        // test attributes
        bundle.AttributeSet("Private", "Some Private Data", function (error) {
            console.log(error);
            bundle.AttributeSet("Public", Buffer.from("Some Public Data"), function (error) {
                console.log(error);
                bundle.AttributeSet("System", "Some System Data", function (error) {
                    console.log(error);
                });
            });
        });
    });



var attrPrivate;
var attrPublic;
var attrSystem;

var pAttrGet = new Promise(
    function (resolve, reject) {
        bundle.AttributeGet("Private", function (error, data) {
            console.log(error);
            attrPrivate = data;
            bundle.AttributeGet("Public", function (error, data) {
                console.log(error);
                attrPublic = data;
                bundle.AttributeGet("System", function (error, data) {
                    console.log(error);
                    attrSystem = data;
                });
            });
        });
    });

pAttrSet.then(function () {
    return pAttrGet;
}).then(function () {
    assert(attrPrivate == "Some Private Data");
    assert(attrPublic == "Some Public Data");
    assert(attrSystem == "Some System Data");
    console.log("Done!");

    // test files
    console.log("Testing files");
    var fileOne = bundle.FileOpen("FileOne.dat", true);
    var fileTwo = bundle.FileOpen("FileTwo.dat", true);
    var fileThree = bundle.FileOpen("FileThree.dat", true);

    assert(fileOne > 0);
    assert(fileTwo > 0);
    assert(fileThree > 0);

    console.log("Testing files attributes");
    bundle.FileAttributeSet(fileOne, "Some Data One");
    bundle.FileAttributeSet(fileTwo, Buffer.from("Some Data Two"));
    bundle.FileAttributeSet(fileThree, "Some Data Three");

    var attrOne = bundle.FileAttributeGet(fileOne);
    var attrTwo = bundle.FileAttributeGet(fileTwo);
    var attrThree = bundle.FileAttributeGet(fileThree);

    assert(attrOne == "Some Data One");
    assert(attrTwo == "Some Data Two");
    assert(attrThree == "Some Data Three");

    console.log("Testing file read/write");

    var block = new Buffer(1024 * 1024);
    for (var i = 0; i < block.length; i++) {
        block[i] = randomIntInc(0, 255)
    }

    for (var i = 0; i < 10; i++) {
        var total = bundle.FileWrite(fileOne, block);
        assert(total == block.length);
    }

    assert(bundle.FileSeek(fileOne, 0, "Set") == 0);

    for (var i = 0; i < 10; i++) {
        var buf = bundle.FileRead(fileOne, block.length);
        assert(buf.length == block.length);
        assert(buf.compare(block) == 0);
    }

    assert(bundle.FileOpen("FileOne.dat", false) == fileOne);
    bundle.FileDelete(fileOne);

    var bundleFiles = bundle.FileNames();
    assert(bundleFiles.length == 2);

    assert(bundle.FileOpen("FileOne.dat", false) == -1);

    console.log("Done!");

});