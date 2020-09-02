var chart = c3.generate({
  bindto: "#scatterChart",
  size: {
    height: 500
  },
  data: {
    // type:'spline',
    x: "x",
    xFormat: '%Y/%m/%d %H:%M:%S',
    // xFormat: '%Y/%m/%d',
    columns:[
      ["x",0],//,'2020/8/18 1:35:45','2020/8/18 1:41:45']


      ["Soil Temp (°C)", 0],
      ["Soil Moisture (%)", 0],
      ["Air Temp (°C)", 0],
      ["Humidity (%)", 0],
      ['Heat Index (°C)', 0],
      ["Sunlight (%)", 0],
      ["Watered Today (0/1)", 0]
    ],
    axes: {
      "Watered Today (0/1)": 'y2' // Add second y-axis for watered_today
    }
  },

  // Axes formatting
  axis: {
    // x axis
    x: {
      type:'timeseries',
      tick: {
        format: '%Y-%m-%d %H:%M:%S',
        rotate: 45
      },
      label: {
        text: 'Date',
        position: 'center'
      }
    },

    // left-hand y axis
    y: {
      label: {
        text: 'Temperature (°C), Percentage (%)',
        position: 'outer-middle'
      }
    },

    // right-hand y axis
    y2: {
      show: true, // add second y-axis
      label: {
        text: "Watered Today (0 = No, 1 = Yes)",
        position: 'outer-middle'
      },
      tick: {
        count: 2, // We don't need float tick values between a "yes" and "no" option
        values: [0,1] // makes sure values aren't floating points
      },
    }
  }
});



// Get data from database, parse into arrays
async function getJSONData() {
    var unix_time = [];
    var date = [];
    var soil_temp = [];
    var soil_moisture = [];
    var air_temp = [];
    var air_humidity = [];
    var heat_index = [];
    var sunlight = [];
    var watered_today = [];
    
    
    date.push("x");
    soil_temp.push("Soil Temp (°C)");
    soil_moisture.push("Soil Moisture (%)");
    air_temp.push("Air Temp (°C)");
    air_humidity.push("Humidity (%)");
    heat_index.push("Heat Index (°C)");
    sunlight.push("Sunlight (%)");
    watered_today.push("Watered Today (0/1)");
    
    
    try {
        $.get("../databaseconnect.php",
        function (data) {
            // alert("success");
            
            // for (var i in data) {
            //     unix_time.push(data[i].unix_time);
            //     date.push(data[i].date);
            //     soil_temp.push(data[i].soil_temp);
            //     soil_moisture.push(data[i].soil_moisture);
            //     air_temp.push(data[i].air_temp);
            //     air_humidity.push(data[i].air_humidity);
            //     heat_index.push(data[i].heat_index);
            //     sunlight.push(data[i].sunlight);
            //     watered_today.push(data[i].watered_today);
            // }
    
        })
        .done(function(data) {
            
            for (var i in data) {
                unix_time.push(data[i].unix_time);
                date.push(data[i].date);
                soil_temp.push(data[i].soil_temp);
                soil_moisture.push(data[i].soil_moisture);
                air_temp.push(data[i].air_temp);
                air_humidity.push(data[i].air_humidity);
                heat_index.push(data[i].heat_index);
                sunlight.push(data[i].sunlight);
                watered_today.push(data[i].watered_today);
            }
            // alert("Second success");
        })
        .fail(function() {
            alert("Error");
        });
    
    return {
            unix_time: unix_time,
            date: date,
            soil_temp: soil_temp,
            soil_moisture: soil_moisture,
            air_temp: air_temp,
            air_humidity: air_humidity,
            heat_index: heat_index,
            sunlight: sunlight,
            watered_today: watered_today
        };
    } catch (err) {
        console.log("Error: " + err);
    }
}


// Graph the data while calling the database every second
var delay = 1000;

async function getGraphData() {
    try {
        setInterval(async function() {
            var results =  await getJSONData();
            
            var mydata = [
                results.date,
                results.soil_temp,
                results.soil_moisture,
                results.air_temp,
                results.air_humidity,
                results.heat_index,
                results.sunlight,
                results.watered_today
            ];
            
            console.log(results.date[1]);
      
            
            chart.load({
                
                columns: mydata             
            });
        }, delay);
        
    } catch(err) {
        console.log("Error: " + err);
    }
}


$(document).ready(function() {
    getGraphData();
});

