/*
  servometer host application
  This nodeJS application handles requesting metrics data from an API
  and sending it over serial USB to the arduino for servo action

  @author Jacob Quatier
*/
var serialport = require('serialport');
var SerialPort = serialport.SerialPort;
var sys = require('sys');
var https = require('https');

function ServoMeter(options) {
  if (!options) throw new Error("ServoMeter options was empty. configure API endpoint and serial port");
  this._serialPort = options.serialPort;
  this._httpOptions = options.httpOptions;
  this.formatSerialData = options.formatSerialData;
}

ServoMeter.prototype.start = function() {
  console.log('starting ServoMeter');
  var port = new SerialPort(this._serialPort, {parser: serialport.parsers.readline('\n')});
  var self = this;
  port.on('open', function() {
    console.log('serial connection opened on ' + self._serialPort);

    // wait 1 second to make sure serial is established
    setTimeout(function() { 
      self.refreshMetrics(port);
    }, 1000)

    // refresh every 10 seconds
    setInterval(function() { 
      self.refreshMetrics(port);
    }, 10000)

    // output any data sent back FROM the arduino (for confirmation)
    port.on('data', function(data){
      console.log('serial incoming -> ', data);
    });
  });

  port.on('error', function(e) {
    console.error('error on serial connection: ', e);
  });
};

ServoMeter.prototype.refreshMetrics = function(port) {
  var self = this;
  // call metrics api using the configured http options object
  var req = https.request(this._httpOptions, function(res) {
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