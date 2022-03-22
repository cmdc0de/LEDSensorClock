import React from 'react';
import logo from './logo.svg'
import { Link } from 'react-router-dom'

const Home = () => {
  return(
        <div>
            <img src={logo} className="App-logo" alt="logo" />
            <h1><Link to={`/scan`}> Scan </Link></h1>
            <h1><Link to={`/sinfo`}> SystemInfo </Link></h1>
            <h1><Link to={'/calibration'}> Calibration Data </Link></h1>
        </div>
    );
}

export default Home;
