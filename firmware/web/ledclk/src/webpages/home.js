import React, {useState, useEffect} from 'react'
import { useDispatch } from 'react-redux'
import logo from './logo.svg'
import { Link } from 'react-router-dom'
import TimezoneSelect from "react-timezone-select";
import moment from "moment-timezone";
import setClockTZ from '../features/tz/tzSlice'

const Home = () => {
  const [tz, setTz] = useState(
    Intl.DateTimeFormat().resolvedOptions().timeZone
  );
  const [datetime, setDatetime] = useState(moment());
  const dispatch = useDispatch()

  useEffect(() => {
    const tzValue = tz.value ?? tz;
    setDatetime(datetime.tz(tzValue));
  }, [tz, datetime]);

  const  postTZ = async () => {
    console.log('You clicked submit.');
    try {
      await dispatch(setClockTZ(JSON.stringify(tz, null, 2))).unwrap()
    } catch (err) {
      console.error('Failed to save the timezone: ', err)
    } finally {
     
    }
  }

  return(
        <div>
            <img src={logo} className="App-logo" alt="logo" />
            <h1><Link to={`/scan`}> Scan </Link></h1>
            <h1><Link to={`/sinfo`}> SystemInfo </Link></h1>
            <h1><Link to={'/calibration'}> Calibration Data </Link></h1>
            <br/>
            <TimezoneSelect value={tz} onChange={setTz} />
            <button onClick={postTZ}>
              Set Time Zone
            </button>

              <div>
                <div>Selected Timezone:</div>
                <pre className="tz-output">{JSON.stringify(tz, null, 2)}</pre>
              </div>
        </div>
    );
}

export default Home;

