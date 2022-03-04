import React, { useEffect }  from 'react';
import { useSelector, useDispatch } from 'react-redux'
import { Link } from 'react-router-dom'
import "../App.css"
import {fetchCalibrationData, calibrationAllData} from '../features/calibration/calibrationSlice'

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
    content = <p>xmin: {calData.xmin}</p>;
  } else if(postStatus==='failed') {
    return <div>Error: {error}</div>;
  }

  return (
    <div>
      {content}
    </div>
  );
}

export default Calibration;

