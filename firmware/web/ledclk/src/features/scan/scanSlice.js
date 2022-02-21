import { createSlice, createAsyncThunk } from '@reduxjs/toolkit'
import { client } from '../../api/client'

const initialState = {
  aps: [],
  status: 'idle',
  error: null,
}

//export const fetchAps = createAsyncThunk('https://my-json-server.typicode.com/cmdc0de/LEDSensorClock/scanresults', async () => {
//  const response = await client.get('https://my-json-server.typicode.com/cmdc0de/LEDSensorClock/scanresults')
//  return response.data
//})

export const fetchAps = () => {
  const response = client.get('https://my-json-server.typicode.com/cmdc0de/LEDSensorClock/scanresults')
  return response.data
}

export const scanSlice = createSlice({
  name: 'scanResults',
  initialState,
  reducers: {
    extraReducers(builder) {
      builder
        .addCase(fetchAps.pending, (state, action) => {
          state.status = 'loading'
        })
        .addCase(fetchAps.fulfilled, (state, action) => {
          state.status = 'succeeded'
          // Add any fetched posts to the array
          state.posts = state.posts.concat(action.payload)
        })
        .addCase(fetchAps.rejected, (state, action) => {
          state.status = 'failed'
          state.error = action.error.message
        })
    },
  }
})

// Action creators are generated for each case reducer function
//export const { } = scanSlice.actions

export default scanSlice.reducer

export const selectAllAPs = (state) => state.scanResults.aps

export const selectPostById = (state, id) => 
  state.scanResults.aps.find((ap) => ap.id === id)


