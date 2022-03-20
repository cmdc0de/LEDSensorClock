import React, { useEffect }  from 'react';
import { useSelector, useDispatch } from 'react-redux'
import { Link } from 'react-router-dom'
import "../App.css"
import {fetchCalibrationData, calibrationAllData} from '../features/calibration/calibrationSlice'

const CalRow = ({ cdata }) => {
  return (
    <tr>
      <td className="tg-e6ik"> {cdata.xmin}</td>
      <td className="tg-e6ik"> {cdata.xmax}</td>
      <td className="tg-e6ik"> {cdata.ymin}</td>
      <td className="tg-e6ik"> {cdata.ymax}</td>
    </tr>
  )
}

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
    content = calData.map((cdata) => (
      <CalRow cdata={cdata} />
    ))
  } else if(postStatus==='failed') {
    return <div>Error: {error}</div>;
  }

  return (
  <div>
  <form action="/resetcal" method="post">
    <table className="tg">
      <thead>
        <tr>
          <td className="tg-e6ik"> xmin</td>
          <td className="tg-e6ik"> xmax</td>
          <td className="tg-e6ik"> ymin</td>
          <td className="tg-e6ik"> ymax</td>
        </tr>
      </thead>
      <tbody>
      {content}
      <tr>
        <td colspan="4"><center> <input type="submit" value="Reset Calibration"/></center></td>
      </tr>
      </tbody>
    </table>
  </form>
  </div>
  );
}

export default Calibration;

