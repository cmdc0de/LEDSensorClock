import { configureStore } from '@reduxjs/toolkit'
import scanReducer from '../features/scan/scanSlice'
import calibrationReducer from '../features/calibration/calibrationSlice'
import sysinfoReducer from '../features/system/sysinfo'

export default configureStore({
  reducer: {
    scanResults: scanReducer,
    calibrationData: calibrationReducer,
    sysinfo: sysinfoReducer
  },
})

