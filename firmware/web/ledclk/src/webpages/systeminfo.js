import React, { useEffect }  from 'react';
import { useSelector, useDispatch } from 'react-redux'
import "../App.css"
import { selectAllsinfo, fetchSysInfo } from '../features/system/sysinfo'
import { Header } from './header'
import { JsonToTable } from "react-json-to-table";

const SInfo  = () => {
  const dispatch = useDispatch()
  const sinfo = useSelector(selectAllsinfo)
  const postStatus = useSelector(state => state.sysinfo.status)
  const error = useSelector(state => state.sysinfo.error)

  useEffect(() => { 
    if (postStatus === 'idle') {
      dispatch(fetchSysInfo())
    }
  }, [postStatus, dispatch])

  let content

  if (postStatus==='loading') {
    content = <div>Loading...</div>;
  } else if (postStatus === 'succeeded') {
    content = <JsonToTable json={sinfo} />
  } else if(postStatus==='failed') {
    return <div>Error: {error}</div>;
  }

  return (
    <div>
    <Header/>
      {content}
    </div>
  );
}

export default SInfo;

