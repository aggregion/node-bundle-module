// process.env.LD_LIBRARY_PATH = __dirname + '/build/Release/';

var rms = require('./build/Release/pfileworker.node');
rms.setOption('verbose', 'off');

module.exports = {
  createPFileAsync: function(data, cb) {
    try {
      var _data = JSON.stringify(data);
    } catch(e) {
      return cb(e);
    }

    return rms.createPFileAsync(_data, cb);
  }
};
