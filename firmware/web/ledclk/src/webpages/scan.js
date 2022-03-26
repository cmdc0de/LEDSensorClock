import React, { useEffect }  from 'react';
import { useSelector, useDispatch } from 'react-redux'
import { Link } from 'react-router-dom'
import "../App.css"
import {selectAllAPs,fetchAps} from '../features/scan/scanSlice'
import { Header } from './header'

const ApRow = ({ ap }) => {
  return (
    <tr>
      <td className="tg-e6ik"> <Link to={`/scan/${ap.id}`}> {ap.id} </Link></td>
      <td className="tg-e6ik"> {ap.ssid}</td>
      <td className="tg-e6ik"> {ap.rssi}</td>
      <td className="tg-e6ik"> {ap.channel}</td>
      <td className="tg-e6ik"> {ap.authMode}</td>
    </tr>
  )
}

const Scan = () => {
  const dispatch = useDispatch()
  const aps = useSelector(selectAllAPs)
  const postStatus = useSelector(state => state.scanResults.status)
  const error = useSelector(state => state.scanResults.error)

  useEffect(() => { 
    if (postStatus === 'idle') {
      dispatch(fetchAps())
    }
  }, [postStatus, dispatch])

  let content

  if (postStatus==='loading') {
    content = <div>Loading...</div>;
  } else if (postStatus === 'succeeded') {
    content = aps.map((ap) => (
      <ApRow key={ap.id} ap={ap} />
    ))
  } else if(postStatus==='failed') {
    return <div>Error: {error}</div>;
  }

  return (
    <div>
    <Header/>
    <table className="tg">
      <thead>
        <tr>
          <td className="tg-e6ik"> id</td>
          <td className="tg-e6ik"> ssid</td>
          <td className="tg-e6ik"> strength</td>
          <td className="tg-e6ik"> channel</td>
          <td className="tg-e6ik"> AuthMode</td>
        </tr>
      </thead>
      <tbody>
      {content}
      </tbody>
    </table>
    </div>
  );
}

export default Scan;

