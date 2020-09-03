/**
 * Run `node install && node app.js`, which will install the programs dependencies
 * and start a server on localhost:3005. The '/' endpoint will generate a random
 * set of data each time its hit.
 */

const express = require('express');
const cors = require('cors');

const MAX_RANDOM_NUMBER = 100;
const NUMBER_OF_DATA_PTS = 15;

const getRandomData = () => {
  const getRandomNumber = () => Math.random() * MAX_RANDOM_NUMBER;

  const data = [];
  for (let i = 0; i < NUMBER_OF_DATA_PTS; i += 1) {
    data.push({
      date: i,
      unix_time: getRandomNumber(),
      soil_temp: getRandomNumber(),
      soil_moisture: getRandomNumber(),
      air_temp: getRandomNumber(),
      air_humidity: getRandomNumber(),
      heat_index: getRandomNumber(),
      sunlight: getRandomNumber(),
      watered_today: getRandomNumber(),
    });
  }
  return data;
};

const app = express();

app.use(cors());

app.get('/', (req, res) => {
  const data = getRandomData();

  res.json(data);
});

app.listen(3005, err => console.log(err || 'listening on 3005'));
