import React from 'react';
import {
  BrowserRouter as Router,
  Switch,
  Routes,
  Route,
  Link
} from "react-router-dom";

import Home from './home';
import WiFiScan from './scan';

const Webpages = () => {
    return(
      <Router>
        <Routes>
            <Route path="/" element={<Home />}></Route>
            <Route path="/scan" element={<WiFiScan />}></Route>
        </Routes>
      </Router>
    );
};

export default Webpages;

