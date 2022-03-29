import React, { useEffect }  from 'react';
import { useSelector, useDispatch } from 'react-redux'
import "../App.css"
import {fetchCalibrationData, calibrationAllData} from '../features/calibration/calibrationSlice'
import { JsonToTable } from "react-json-to-table";
import { Header } from './header'

const Calibration = () => {
  const dispatch = useDispatch()
  const calData = useSelector(calibrationAllData)
  const postStatus = useSelector(state => state.calibrationData.status)
  const error = useSelector(state => state.calibrationData.error)

  useEffect(() => { 
    if (postStatus === 'idle') {
      dispatch(fetchCalibrationData())
    }
  }, [postStatus, dispatch])

  let content

  if (postStatus==='loading') {
    content = <div>Loading...</div>;
  } else if (postStatus === 'succeeded') {
    content = <JsonToTable json={calData} />
  } else if(postStatus==='failed') {
    return <div>Error: {error}</div>;
  }

  return (
  <div>
    <Header/>
  <form action="/resetcal" method="post">
    {content}
    <input type="submit" value="Reset Calibration"/>
  </form>
  </div>
  );
}

export default Calibration;

