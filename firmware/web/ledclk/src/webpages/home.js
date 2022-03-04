import React from 'react';
import { Counter } from '../features/counter/Counter'
import logo from './logo.svg'

const Home = () => {
  return(
        <div>
            <img src={logo} className="App-logo" alt="logo" />
            <h1><a href="/scan"> Scan</a></h1>
            <h1><a href="/calibration"> Calibration Data </a></h1>
            <Counter/>
        </div>
    );
}

export default Home;
