import { createSlice, createAsyncThunk } from '@reduxjs/toolkit'
import { client } from '../../api/client'

const initialState = {
  aps: [],
  status: 'idle',
  error: null,
}

export const fetchAps = createAsyncThunk('scanResults/fetchAps', async () => {
  var uri = '/wifiscan';
  if (process.env.NODE_ENV !== 'production') {
    uri = 'http://localhost:5000/wifiscan';
  }
  const response = await client.get(uri);
  return response.data
})

export const scanSlice = createSlice({
  name: 'scanResults',
  initialState,
  reducers: {
  },
  extraReducers(builder) {
    builder
      .addCase(fetchAps.pending, (state, action) => {
        state.status = 'loading'
      })
      .addCase(fetchAps.fulfilled, (state, action) => {
        state.status = 'succeeded'
        // Add any fetched posts to the array
        state.aps = state.aps.concat(action.payload)
      })
      .addCase(fetchAps.rejected, (state, action) => {
        state.status = 'failed'
        state.error = action.error.message
      })
  }
})

// Action creators are generated for each case reducer function
//export const { } = scanSlice.actions

export default scanSlice.reducer

export const selectAllAPs = (state) => state.scanResults.aps

export const selectAPById = (state, id) => state.scanResults.aps.find((ap) => ap.id === id)

