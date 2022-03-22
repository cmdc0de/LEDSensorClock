import React from 'react';
import {
  BrowserRouter as Router,
  Routes,
  Route,
} from "react-router-dom";

import Home from './home';
import WiFiScan from './scan';
import ScanDetails from './scandetails'
import Calibration from './calibration'
import SysInfo from './systeminfo'

const Webpages = () => {
    return(
      <Router>
        <Routes>
            <Route path="/" element={<Home />}></Route>
            <Route path="/scan" element={<WiFiScan />}> </Route>
            <Route exact path="/scan/:apid" element={<ScanDetails />}></Route>
            <Route exact path="/calibration" element={<Calibration />}></Route>
            <Route exact path="/sinfo" element={<SysInfo />}></Route>
        </Routes>
      </Router>
    );
};

export default Webpages;

