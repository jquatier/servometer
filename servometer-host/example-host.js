var ServoMeter = require('./lib/servometer.js');

/*
  Sample configuration that pulls data for a specific application ID in New Relic
  Includes function for formatting serial data to send to the arduino, this allows 
  implementations to pull what fields are needed from their API response.

  @author Jacob Quatier
*/
var meterOptions = {
  serialPort: '/dev/tty.usbmodem1411',
  httpOptions: {
    hostname: 'api.newrelic.com',
    port: 443,
    path: '/v2/applications/<YOUR_APP_ID>.json',
    method: 'GET',
    headers: { 'X-Api-Key': '<YOUR_API_KEY>' }
  },
  formatSerialData: function(response) {
    var responseObj = JSON.parse(response);

    // grab the metrics we need
    var throughput = responseObj.application.application_summary.throughput.toString();
    if(throughput.length > 4)
    {
      // format numbers over 4 digits
      throughput = throughput.substring(0,2) + "k";
    }

    //build a string with RESPONSETIME:THROUGHPUT:HEALTH
    return responseObj.application.application_summary.response_time.toString() + ':' +
      throughput + ':' +
      responseObj.application.health_status.toString();
  }
};

var meter = new ServoMeter(meterOptions);
meter.start();