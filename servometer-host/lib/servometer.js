
const Readline = require('@serialport/parser-readline')
const SerialPort = require('serialport'),
  https = require('https');

function handleSerialData(data) {
  console.log('serial incoming -> ', data);
}

function handleSerialError(err) {
  console.error('error on serial connection: ', err);
}

/*
  servometer host application
  This nodeJS application handles requesting metrics data from an API
  and sending it over serial USB to the arduino for servo action

  @author Jacob Quatier
*/
function ServoMeter(options) {
  if (!options) {
    throw new Error("ServoMeter options was empty. configure API endpoint and serial port");
  }

  this._serialPort = options.serialPort;
  this._httpOptions = options.httpOptions;
  this.formatSerialData = options.formatSerialData;
}

ServoMeter.prototype.start = function() {
  var self = this;
  console.log('starting ServoMeter');

  const port = new SerialPort(self._serialPort);
  port.pipe(new Readline({ delimiter: '\n' }))
  function refreshMetrics(){
    self.refreshMetrics(port);
  }

  port.on('open', function() {
    console.log('serial connection opened on ' + self._serialPort);

    // wait 1 second to make sure serial is established
    setTimeout(refreshMetrics, 1000)

    // refresh every 10 seconds
    setInterval(refreshMetrics, 10000)
  });

  // output any data sent back FROM the arduino (for confirmation)
  port.on('data', handleSerialData);

  port.on('error', handleSerialError);
};

ServoMeter.prototype.refreshMetrics = function(port) {
  var self = this;
  // call metrics api using the configured http options object
  var req = https.request(self._httpOptions, function(res) {
    console.log('recieved API status code: ', res.statusCode, ' -- ', res.headers.date);
    var response = '';

    // buffer the response until it ends
    res.on('data', function(data) {
      response += data;
    });

    res.on('end', function() {
      // format & write out the string
      port.write(self.formatSerialData(response));
    });
  });

  req.on('error', function(e) {
    console.error('Error requesting metrics: ', e);
  });
  
  req.end();
}

module.exports = ServoMeter;
