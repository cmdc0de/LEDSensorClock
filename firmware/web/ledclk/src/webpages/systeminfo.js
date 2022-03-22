import React, { useEffect }  from 'react';
import { useSelector, useDispatch } from 'react-redux'
import "../App.css"
import { selectAllsinfo, fetchSysInfo } from '../features/system/sysinfo'
import { Header } from './header'

const SinfoRow = ({ r }) => {
  return (
  //  for(let x in r) {
      <tr>
        <td className="tg-e6ik"> x    </td>
        <td className="tg-e6ik"> r[x] </td>
      </tr>
//    }
  )
}

const SInfo  = () => {
  const dispatch = useDispatch()
  const sinfo = useSelector(selectAllsinfo)
  const postStatus = useSelector(state => state.scanResults.status)
  const error = useSelector(state => state.scanResults.error)

  useEffect(() => { 
    if (postStatus === 'idle') {
      dispatch(fetchSysInfo())
    }
  }, [postStatus, dispatch])

  let content

  if (postStatus==='loading') {
    content = <div>Loading...</div>;
  } else if (postStatus === 'succeeded') {
    content = sinfo.map((row) => (
      <SinfoRow r={row} />
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
          <td className="tg-e6ik"> Name</td>
          <td className="tg-e6ik"> Value</td>
        </tr>
      </thead>
      <tbody>
      {content}
      </tbody>
    </table>
    </div>
  );
}

export default SInfo;

