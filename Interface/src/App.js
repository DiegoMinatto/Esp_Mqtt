import './App.css';
import React, { Component } from 'react';
import { FaTemperatureHigh } from 'react-icons/fa';
import {
  LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, Legend,ResponsiveContainer
} from 'recharts';
import socketIOClient from "socket.io-client";
const axios = require('axios');


const ENDPOINT = "http://127.0.0.1:8080";

const renderCustomAxisTick = (name) => {
  console.log()
 return `${name} Cº`
}

class App extends Component {

  constructor(props){
    super(props);
    this.state = {
        grafico: [],
        tempAtual: 0,
        ultimaAtualizacao: ''
    }



    const socket = socketIOClient(ENDPOINT, { transport : ['websocket'] });
    socket.on("FromAPI", data => {
      this.setState({grafico: data.grafico, tempAtual:data.atual.Temperatura, ultimaAtualizacao: data.atual.Hora});
    
    });
}



  componentDidMount(){


  }

  renderColorfulLegendText(value, entry) {
  	const { color } = entry;
    
    return <span className="color">{value}</span>;
  }




  render() {    
    return (
      <div className="App">
        <div className="Nav">
          <span>Temperatura IOT</span>
        </div>

        <div className="container">
          <div className="temp">
       
          
          <div className="ContainerTemp">
          <p className="bold">Temperatura Atual:</p>
            <div className="TempAtual">
               <FaTemperatureHigh color={'#4dcd84'} size={50} /> <span className="TempInfo">{this.state.tempAtual} Cº</span>
               
            </div>
            <p>Última atualização: {this.state.ultimaAtualizacao}</p>
          </div>
          </div>



          <div className="grafico">
            <p className="bold">Variação média (24h):</p>
          <ResponsiveContainer width="95%" height={400}>
             <LineChart
       
        data={this.state.grafico}
        margin={{
          top: 5, right: 30, left: 20, bottom: 5,
        }}
      >
        <CartesianGrid stroke="gray" strokeDasharray="3 3" />
        <XAxis stroke="white" dataKey="Hora" />
        <YAxis stroke="white" tickFormatter={renderCustomAxisTick} />
       
       
        <Line type="monotone" dataKey="Temperatura" stroke="#4dcd84" activeDot={{ r: 8 }} />
    
      </LineChart>
      </ResponsiveContainer>
          </div>

        </div>

      </div>
    );
  }
}

export default App;
