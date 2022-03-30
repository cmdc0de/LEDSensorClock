import { createSlice, createAsyncThunk } from '@reduxjs/toolkit'
import { client } from '../../api/client'

const initialState = {
  sData: [],
  status: 'idle',
  error: null,
}

export const fetchSettings = createAsyncThunk('setting/fetchSettings', async () => {
  var uri = '/setting';
  if (process.env.NODE_ENV !== 'production') {
    uri = 'http://localhost:5000/setting';
  }
  const response = await client.get(uri);
  return response.data
})

export const settingSlice = createSlice({
  name: 'settingData',
  initialState,
  reducers: {
  },
  extraReducers(builder) {
    builder
      .addCase(fetchSettings.pending, (state, action) => {
        state.status = 'loading'
      })
      .addCase(fetchSettings.fulfilled, (state, action) => {
        state.status = 'succeeded'
        // Add any fetched posts to the array
        state.sData = state.sData.concat(action.payload)
      })
      .addCase(fetchSettings.rejected, (state, action) => {
        state.status = 'failed'
        state.error = action.error.message
      })
  }
})

// Action creators are generated for each case reducer function
//export const { } = scanSlice.actions

export default settingSlice.reducer

export const selectAllSettings = (state) => state.settingData.sData


