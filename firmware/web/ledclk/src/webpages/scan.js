import React, { useState, useEffect }  from 'react';
import "../App.css"

const Home = () => {
  const [error, setError] = useState(null);
    const [isLoaded, setIsLoaded] = useState(false);
    const [aps, setAps] = useState([]);    

  useEffect(() => {
        fetch("https://my-json-server.typicode.com/cmdc0de/LEDSensorClock/scanresults")
            .then(res => res.json())
            .then(
                (data) => {
                    setIsLoaded(true);
                    setAps(data);
                },
                (error) => {
                    setIsLoaded(true);
                    setError(error);
                }
            )
      }, [])

  if (error) {
        return <div>Error: {error.message}</div>;
    } else if (!isLoaded) {
        return <div>Loading...</div>;
    } else {
      return (
        <table class="tg">
          <thead>
              <tr>
                <td class="tg-e6ik"> id</td>
                <td class="tg-e6ik"> ssid</td>
                <td class="tg-e6ik"> strength</td>
                <td class="tg-e6ik"> channel</td>
                <td class="tg-e6ik"> AuthMode</td>
              </tr>
          </thead>
            {aps.map(ap => (
              <tr>
                <td class="tg-e6ik"> {ap.id}</td>
                <td class="tg-e6ik"> {ap.ssid}</td>
                <td class="tg-e6ik"> {ap.rssi}</td>
                <td class="tg-e6ik"> {ap.channel}</td>
                <td class="tg-e6ik"> {ap.authMode}</td>
              </tr>
                ))}
          </table>
        );
    }
}

export default Home;

