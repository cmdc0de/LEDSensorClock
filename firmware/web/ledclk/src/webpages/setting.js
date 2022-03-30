import React, { useEffect }  from 'react';
import { useSelector, useDispatch } from 'react-redux'
import "../App.css"
import { selectAllSettings, fetchSettings } from '../features/setting/settingSlice'
import { Header } from './header'

const SettingRow = ({ r }) => {
  return (
    <div>
    <form action="/setsetting" method="post">
    {r.name}:  <input type="hidden" value={r.name}/><input type="number" min="0" max="9999" defaultValue={r.value}/><input type="submit" value="Change Value"/><br/><br/>
    </form>
    </div>
  )
}

const Setting = () => {
  const dispatch = useDispatch()
  const settingList = useSelector(selectAllSettings)
  const postStatus = useSelector(state => state.settingData.status)
  const error = useSelector(state => state.settingData.error)

  useEffect(() => { 
    if (postStatus === 'idle') {
      dispatch(fetchSettings())
    }
  }, [postStatus, dispatch])

  let content

  if (postStatus==='loading') {
    content = <div>Loading...</div>;
  } else if (postStatus === 'succeeded') {
    content = settingList.map((r) => (
      <SettingRow key={r.name} r={r} />
    ))
  } else if(postStatus==='failed') {
    return <div>Error: {error}</div>;
  }

  return (
    <div>
    <Header/>
    <br/>
    <br/>
      {content}
    </div>
  );
}

export default Setting;

