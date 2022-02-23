import React, {useState} from 'react'
import { useSelector } from 'react-redux'
//import { selectAPById } from '../features/scan/scanSlice'
import { selectAllAPs } from '../features/scan/scanSlice'
import { useParams } from 'react-router-dom'

const ScanDetails = () => {
  const { apid } = useParams();
  const aps = useSelector(selectAllAPs)
  //const ap = useSelector(state => selectAPById(state, apid))
  const ap = aps.find(a => a.id == apid);
  const canSave = true
  const [addRequestStatus, setAddRequestStatus] = useState('idle')

  const onSavePostClicked = async () => {
    if (canSave) {
      try {
        setAddRequestStatus('pending')
        console.log('saving data.....')
      } catch (err) {
        console.error('Failed to save the post: ', err)
      } finally {
        setAddRequestStatus('idle')
      }
    }
  }

  if (!ap) {
    return (
      <section>
        <h2>Post not found!</h2>
      </section>
    )
  }

  return (
    <section>
      <article >
        <h2>{ap.ssid}</h2><br/>
        <div>
          <form>
            <table>
              <tbody>
                <tr><td> ID: </td><td>{ap.id}</td></tr>
                <tr><td> Channel: </td><td>{ap.channel}</td></tr>
                <tr><td> RSSI: </td><td>{ap.rssi}</td></tr>
                <tr><td> Auth Type: </td><td>{ap.authMode}</td></tr>
                <tr>
                  <td colSpan="2"><input type="password" id="pass" name="password" size="32" maxLength="64" required/></td>
                </tr>
                <tr>
                    <td colSpan="2"><center><button type="button" onClick={onSavePostClicked}> Connect </button></center></td>
                </tr>
              </tbody>
            </table>
          </form>
        </div>
      </article>
    </section>
  )
}

export default ScanDetails;
