import React from 'react'
import { useSelector } from 'react-redux'
import { selectAllAPs } from '../features/scan/scanSlice'
import { useParams } from 'react-router-dom'
import { Header } from './header'

const ScanDetails = () => {
  const { apid } = useParams();
  const aps = useSelector(selectAllAPs)
  const ap = aps.find(a => a.id == apid);

  if (!ap) {
    return (
      <section>
        <h2>Post not found!</h2>
      </section>
    )
  }

  return (
    <div>
    <Header/>
    <section>
      <article >
        <h2>{ap.ssid}</h2><br/>
        <div>
          <form action="/setcon" method="post">
            <input type="hidden" name="id" value={ap.id}/>
            <table>
              <tbody>
                <tr><td> ID: </td><td>{ap.id}</td></tr>
                <tr><td> Channel: </td><td>{ap.channel}</td></tr>
                <tr><td> RSSI: </td><td>{ap.rssi}</td></tr>
                <tr><td> Auth Type: </td><td>{ap.authMode}</td></tr>
                <tr>
                  <td colSpan="2">
                      <input type="password" id="pass" name="password" 
                      size="32" maxLength="64" required/>
                  </td>
                </tr>
                <tr>
                    <td colspan="2"><center><input type="submit" value="Connect"/></center></td>
                </tr>
              </tbody>
            </table>
          </form>
        </div>
      </article>
    </section>
    </div>
  )
}

export default ScanDetails;
