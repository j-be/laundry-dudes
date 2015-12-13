ON_THRESHOLD = 500;

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

getData = function() {
  sendRequest("laundrydude/api/data", 'GET').done(function(data) {
    var chart_data = null;

    showHumidity(getLast(data['h']));
    showLight(getLast(data['l']));

    console.log('humid');
    humidity_chart_data = initChartData('Humidity', data['h']);
    humidity_chart.draw(humidity_chart_data, humidity_options);

    console.log('light');
    light_chart_data = initChartData('Light', data['l']);
    light_chart.draw(light_chart_data, light_options);
  })
}

showHumidity = function(humidity_data) {
  var humidity_value = null;

  if (humidity_data)
    humidity_value = humidity_data[1] + " %"

  showData(humidity_value, '#humidity');
}

showLight = function(light_data) {
  var on_state = null;

  if (light_data[1] > ON_THRESHOLD)
    on_state = ('ON');
  else
    on_state = ('OFF');

  showData(on_state, '#light');
}

showData = function(data, tag_id) {
  if (data)
    $(tag_id + "-value").text(data);
  else
    $(tag_id + "-value").text("No data");
}

formatTime = function(unix_timestamp) {
  var date = new Date(unix_timestamp*1000);

  var hours = "0" + date.getHours();
  var minutes = "0" + date.getMinutes();
  var seconds = "0" + date.getSeconds();

  //return hours + ':' + minutes.substr(-2) + ':' + seconds.substr(-2);
  return [date.getHours() % 24, date.getMinutes(), date.getSeconds()];
}

var humidity_chart = null;
var common_options = {
  legend: {
    position: 'none'
  },
  hAxis: {
    viewWindow: {
      min: [0, 0, 0,],
      max: [24,0,0]
    }
  }
};

var light_options = jQuery.extend(true, {}, common_options);
light_options.colors = ['#558B2F'];
light_options.vAxis = {
  viewWindow: {
      min: 0,
      max: 1000
  }
};


var humidity_options = jQuery.extend(true, {}, common_options);
humidity_options.colors = ['#0277BD'];
humidity_options.vAxis = {
  viewWindow: {
      min: 0,
      max: 100
  }
};

google.load('visualization', '1.1', {packages: ['corechart', 'line']});
google.setOnLoadCallback(drawBackgroundColor);

function initChartData(column_name, rows) {
  var chart_data = new google.visualization.DataTable();

  chart_data.addColumn('timeofday', 'Time of day');
  chart_data.addColumn('number', column_name);

  chart_data.addRows(rows);

  return chart_data;
}

function drawBackgroundColor() {
  humidity_chart = new google.visualization.LineChart(
    document.getElementById('humidity_chart_div'));
  light_chart = new google.visualization.LineChart(
    document.getElementById('light_chart_div'));

  getData();
  window.setInterval(getData, 5000);
}

