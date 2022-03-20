import React from 'react';
import { Counter } from '../features/counter/Counter'
import logo from './logo.svg'
import { Link } from 'react-router-dom'

const Home = () => {
  return(
        <div>
            <img src={logo} className="App-logo" alt="logo" />
            <h1><Link to={`/scan`}> Scan </Link></h1>
            <h1><a href="/calibration"> Calibration Data </a></h1>
            <Counter/>
        </div>
    );
}

export default Home;
