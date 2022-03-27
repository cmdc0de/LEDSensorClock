import { createSlice, createAsyncThunk } from '@reduxjs/toolkit'
import { client } from '../../api/client'

const initialState = {
  calData: {},
  status: 'idle',
  error: null,
}

export const fetchCalibrationData = createAsyncThunk('calibrationData/fetchCalibrationData', async () => {
  var uri = '/calibration';
  if (process.env.NODE_ENV !== 'production') {
    uri = 'http://localhost:5000/calibration';
  }
  const response = await client.get(uri);
  return response.data
})

export const calibrationSlice = createSlice({
  name: 'calibrationData',
  initialState,
  reducers: {
  },
  extraReducers(builder) {
    builder
      .addCase(fetchCalibrationData.pending, (state, action) => {
        state.status = 'loading'
      })
      .addCase(fetchCalibrationData.fulfilled, (state, action) => {
        state.status = 'succeeded'
        // Add any fetched posts to the array
        state.calData = action.payload
      })
      .addCase(fetchCalibrationData.rejected, (state, action) => {
        state.status = 'failed'
        state.error = action.error.message
      })
  }
})

// Action creators are generated for each case reducer function
//export const { } = scanSlice.actions

export default calibrationSlice.reducer

export const calibrationAllData = (state) => state.calibrationData.calData

