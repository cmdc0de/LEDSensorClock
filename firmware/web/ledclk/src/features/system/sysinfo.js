import { createSlice, createAsyncThunk } from '@reduxjs/toolkit'
import { client } from '../../api/client'

const initialState = {
  sinfo: {},
  status: 'idle',
  error: null,
}

export const fetchSysInfo = createAsyncThunk('sysinfo/FetchSysInfo', async () => {
  var uri = '/systeminfo';
  if (process.env.NODE_ENV !== 'production') {
    uri = 'http://localhost:5000/systeminfo';
  }
  const response = await client.get(uri);
  return response.data
})

export const sysinfoSlice = createSlice({
  name: 'sysinfo',
  initialState,
  reducers: {
  },
  extraReducers(builder) {
    builder
      .addCase(fetchSysInfo.pending, (state, action) => {
        state.status = 'loading'
      })
      .addCase(fetchSysInfo.fulfilled, (state, action) => {
        state.status = 'succeeded'
        // Add any fetched posts to the array
        //state.sinfo = state.sinfo.concat(action.payload)
        state.sinfo = action.payload
      })
      .addCase(fetchSysInfo.rejected, (state, action) => {
        state.status = 'failed'
        state.error = action.error.message
      })
  }
})

// Action creators are generated for each case reducer function
//export const { } = scanSlice.actions

export default sysinfoSlice.reducer

export const selectAllsinfo = (state) => state.sysinfo.sinfo


