import React from 'react';
import { Link } from 'react-router-dom'

export function Header() {
  return (
        <div>
          <table>
            <tbody>
              <tr>
                <td><Link to={`/`}> Home </Link> </td>
                <td><Link to={`/scan`}> Scan </Link> </td>
                <td><Link to={`/sinfo`}> SystemInfo </Link></td>
                <td><Link to={'/calibration'}> Calibration Data </Link></td>
                <td><Link to={'/setting'}> Settings </Link></td>
              </tr>
            </tbody>
          </table>
        </div>
    )
}

