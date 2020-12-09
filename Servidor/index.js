const express = require('express')
const fs = require('fs');

const bodyParser = require('body-parser')
const app = express()
var cors = require('cors')
const port = 8080
var mqtt = require('mqtt')

var certFile = fs.readFileSync("D:\\Programas\\mosquitto\\cert\\servidor.crt");

var sqlite3 = require('sqlite3').verbose();
var db = new sqlite3.Database('./tempDB.db');

var opts = {
  rejectUnauthorized: false,
  username: 'servidor',
  password: 's3rv1d0r.1',
  cert: certFile,
}

const client = new mqtt.connect('mqtts://Diego', opts);

client.on('connect', function () {
  client.subscribe('/topic/qos0', function (err) {
    console.log(err)
  })
})

client.on('message', function (topic, message) {
  if(topic === '/topic/qos0'){
    if(message.toString()){
      var stmt = db.prepare("insert into leituras(temperatura) values (?)");
      stmt.run(Number(message.toString()));
      stmt.finalize();
    }
  }  
})


app.use(cors())
app.use(bodyParser.json({
  extended: true,
  limit:'50mb'
}))
app.use(
  bodyParser.urlencoded({
    extended: true,
    limit:'50mb'
  })
)

async function getDados(request, response){

    db.all("SELECT round(avg(temperatura)) as Temperatura,strftime('%H',datetime(timestamp, 'localtime')) || ':00' as Hora from leituras where datetime(timestamp, 'localtime') >= datetime(datetime('now', '-1 days'), 'localtime') AND datetime(timestamp, 'localtime') < datetime(datetime('now'), 'localtime') group by strftime('%H',datetime(timestamp, 'localtime'))", [], (err, resultGrafico) => {
      if (err) {
        return console.error(err.message);
      }
      db.get("SELECT temperatura as Temperatura,strftime('%H:%M %d-%m-%Y',datetime(timestamp, 'localtime')) as Hora from leituras where id_leitura = (select max(leit.id_leitura) from leituras leit)", [], (err, resultAtual) => {
        if (err) {
          return console.error(err.message);
        }    
        var obj = {
          grafico: resultGrafico,
          atual: resultAtual
        }
        response.status(200).json(obj) 
      });

      
    
    });

      
  
 
  
  
 
 }

app.get('/temperatura', getDados)



const server = app.listen(port, () => {
    console.log(`App running on port ${port}.`)
})



const io = require('socket.io')(server, {
  cors: {
    origin: '*'
  }
});



let interval;

io.on("connection", (socket) => {
  console.log("Conectado");
  if (interval) {
    clearInterval(interval);
  }
  interval = setInterval(() => getApiAndEmit(socket), 1000);
  socket.on("disconnect", () => {
    console.log("Disconectado");
    clearInterval(interval);
  });
});

const getApiAndEmit = socket => {

  db.all("SELECT round(avg(temperatura)) as Temperatura,strftime('%H',datetime(timestamp, 'localtime')) || ':00' as Hora from leituras where datetime(timestamp, 'localtime') >= datetime(datetime('now', '-1 days'), 'localtime') AND datetime(timestamp, 'localtime') < datetime(datetime('now'), 'localtime') group by strftime('%H',datetime(timestamp, 'localtime'))", [], (err, resultGrafico) => {
    if (err) {
      return console.error(err.message);
    }
    db.get("SELECT temperatura as Temperatura,strftime('%H:%M %d-%m-%Y',datetime(timestamp, 'localtime')) as Hora from leituras where id_leitura = (select max(leit.id_leitura) from leituras leit)", [], (err, resultAtual) => {
      if (err) {
        return console.error(err.message);
      }    
      var obj = {
        grafico: resultGrafico,
        atual: resultAtual
      }
      socket.emit("FromAPI", obj);
    });

    
  
  });
  // Emitting a new message. Will be consumed by the client
  
};