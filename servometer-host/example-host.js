var ServoMeter = require('./lib/servometer.js'),
  meterOptions,
  meter;

/*
  Sample configuration that pulls data for a specific application ID in New Relic
  Includes function for formatting serial data to send to the arduino, this allows
  implementations to pull what fields are needed from their API response.

  @author Jacob Quatier
*/
meterOptions = {
  serialPort: '/dev/tty.usbmodem1411',
  httpOptions: {
    hostname: 'api.newrelic.com',
    port: 443,
    path: '/v2/applications/<YOUR_APP_ID>.json',
    method: 'GET',
    headers: { 'X-Api-Key': '<YOUR_API_KEY>' }
  },
  formatSerialData: function(response) {
    var responseObj = JSON.parse(response),
      applicationSummary = responseObj.application.application_summary,
      throughput = applicationSummary.throughput.toString(),
      healthStatus = applicationSummary.health_status.toString(),
      responseTime = applicationSummary.response_time.toString();

    // grab the metrics we need
    if (throughput.length > 4) {
      // format numbers over 4 digits
      throughput = throughput.substring(0,2) + "k";
    }

    //build a string with RESPONSETIME:THROUGHPUT:HEALTH
    return [responseTime, throughput, healthStatus].join(':');
  }
};

meter = new ServoMeter(meterOptions);
meter.start();
