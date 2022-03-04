import { configureStore } from '@reduxjs/toolkit'
import counterReducer from '../features/counter/counterSlice'
import scanReducer from '../features/scan/scanSlice'
import calibrationReducer from '../features/calibration/calibrationSlice'

export default configureStore({
  reducer: {
    counter: counterReducer,
    scanResults: scanReducer,
    calibrationData: calibrationReducer,
  },
})

