ON_THRESHOLD = 450;

STATE_NAMES = ["Off", "Idle", "Washing", "Centrifuging", "Done"]

sendRequest = function(uri, method, data, is_async) {
  var request = {
    url: uri,
    type: method,
    contentType: "application/json",
    accepts: "application/json",
    cache: false,
    dataType: 'json',
    async: is_async,
    data: JSON.stringify(data),
    error: function(jqXHR) {
      console.log("ajax error " + jqXHR.status);
    }
  };
  return $.ajax(request);
}

getLast = function(list) {
  return list[list.length - 1];
}

getLastData = function() {
  sendRequest("api/last-data", 'GET').done(function(data) {
    var chart_data = null;

    showHumidity(data['h'], data['t']);

    state = data['s'];
    console.log(state);
    showData(state[0], '#state-timestamp');
    showData(STATE_NAMES[state[1]], '#state-value');

    showData(data['r'][1], "#user-name");
  })
}

showHumidity = function(humidity_data, temperature_data) {
  var humidity_value = null;

  if (humidity_data)
    humidity_string = humidity_data[1] + " %"
  if (temperature_data)
    temperature_string = temperature_data[1] + " °C"

  showData(humidity_string, '#humidity-value');
  showData(temperature_string, '#temperature-value');
}

showData = function(data, tag_id) {
  if (data)
    $(tag_id).text(data);
  else
    $(tag_id).text("No data");
}

getLastData();
window.setInterval(getLastData, 5000);
