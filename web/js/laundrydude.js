ON_THRESHOLD = 450;

STATE_NAMES = ["Off", "Idle", "Washing", "Spin-drying", "Done"]

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
    var lockState = "Free";
    var reservation = undefined;

    showHumidity(data['h'], data['t']);

    state = data['s'];
    showData(state[0], '#state-timestamp');
    showData(STATE_NAMES[state[1]], '#state-value');

    if (data['u'] != undefined)
      lockState = "Locked by " + data['u'];

    if (data['r'] == undefined)
      showData("No reservations pending...", "#next-reservation")
    else {
      showData(data['r'].user + " at " + data['r'].start, "#next-reservation")
      nextReservationStart = new Date(data['r'].startTs * 1000);
      twoHoursBefore = new Date(nextReservationStart.getTime()).addHours(-2);
      if (new Date().between(twoHoursBefore, nextReservationStart))
        lockState = "Reserved for " + data['r'].user;
    }

    showData(lockState, "#user-name");
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
