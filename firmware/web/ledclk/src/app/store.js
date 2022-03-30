import { configureStore } from '@reduxjs/toolkit'
import scanReducer from '../features/scan/scanSlice'
import calibrationReducer from '../features/calibration/calibrationSlice'
import sysinfoReducer from '../features/system/sysinfo'
import tzReducer from '../features/tz/tzSlice'
import settingReducer from '../features/setting/settingSlice'

export default configureStore({
  reducer: {
    scanResults: scanReducer,
    calibrationData: calibrationReducer,
    sysinfo: sysinfoReducer,
    tzData: tzReducer,
    settingData: settingReducer
  },
})

