#!/usr/bin/env node

const cli = require('cli');
const check = require('check-types');
const Bundle = require('../index');

cli.parse({
    path: ['p', 'Relative path to the file in the bundle', 'string']
}, ['props', 'info', 'fileprops', 'file']);

cli.main((args, options) => {
    try {
        check.assert.hasLength(args, 1, 'You must specify one input file');
        let bundle = new Bundle({path: args[0], readonly: true});
        let command = cli.command;
        let promise;
        switch (command) {
            case 'props':
                promise = bundle.getBundlePropertiesData();
                break;
            case 'info':
                promise = bundle.getBundleInfoData();
                break;
            case 'fileprops':
                check.assert.assigned(options.path, 'You must specify path to the file in the bundle (-p option)');
                check.assert.nonEmptyString(options.path, 'Path should be non-empty string');
                promise = bundle.readFilePropertiesData(bundle.openFile(options.path));
                break;
            case 'file':
                check.assert.assigned(options.path, 'You must specify path to the file in the bundle (-p option)');
                check.assert.nonEmptyString(options.path, 'Path should be non-empty string');
                let size = bundle.getFileSize(options.path);
                promise = bundle.readFileBlock(bundle.openFile(options.path), size);
                break;
            default:
                throw new Error('Unknown command');
        }
        promise
            .then((buffer) => {
                console.log(buffer.toString('hex'));
            })
            .catch(cli.fatal);
    } catch (e) {
        cli.fatal(e);
    }
});