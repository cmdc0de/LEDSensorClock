import { createSlice, createAsyncThunk } from '@reduxjs/toolkit'
import { client } from '../../api/client'

const initialState = {
  tz: { "value": "America/Phoenix", "label": "(GMT-7:00) Arizona", "offset": -7, "abbrev": "MST",
      "altName": "Mountain Standard Time" },
  status: 'idle',
  error: null,
}

export const fetchTZ = createAsyncThunk('sysinfo/FetchTZ', async() => {
  var uri = '/tz';
  if (process.env.NODE_ENV !== 'production') {
    uri = 'http://localhost:5000/tz';
  }
  const response = await client.get(uri);
  return response.data
})

export const setClockTZ  = createAsyncThunk('posts/setClockTZ', async (tzData) => {
    var uri = '/settz';
    if (process.env.NODE_ENV !== 'production') {
      uri = 'http://localhost:5000/settz';
    }
    const response = await client.post(uri, tzData)
    return response.data
  }
)

export const tzSlice = createSlice({
  name: 'tz',
  initialState,
  reducers: {
  },
  extraReducers(builder) {
    builder
      .addCase(fetchTZ.pending, (state, action) => {
        state.status = 'loading'
      })
      .addCase(fetchTZ.fulfilled, (state, action) => {
        state.status = 'succeeded'
        // Add any fetched posts to the array
        state.tz = action.payload
      })
      .addCase(fetchTZ.rejected, (state, action) => {
        state.status = 'failed'
        state.error = action.error.message
      })
      .addCase(setClockTZ.fulfilled, (state, action) => {
        state.status = 'succeeded'
        console.log(action.payload);
        state.tz = action.payload
      })
  }
})

// Action creators are generated for each case reducer function
//export const { } = scanSlice.actions

export default tzSlice.reducer

export const selectTZ = (state) => state.tz.tz

