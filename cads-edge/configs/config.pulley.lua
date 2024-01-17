json = require "json"

gocatorFps = 984.0

iirfilter = {
  Skip = 10000,
  Delay = 334,
  Sos  = {{ 1.66008519e-07,  1.66008519e-07,  0.00000000e+00, 1.00000000e+00, -9.77361093e-01,  0.00000000e+00},
          { 1.00000000e+00, -1.98638977e+00,  1.00000000e+00, 1.00000000e+00, -1.95693573e+00,  9.57429859e-01},
          { 1.00000000e+00, -1.99649383e+00,  1.00000000e+00, 1.00000000e+00, -1.96256904e+00,  9.63016716e-01},
          { 1.00000000e+00, -1.99836739e+00,  1.00000000e+00, 1.00000000e+00, -1.96958098e+00,  9.69971891e-01},
          { 1.00000000e+00, -1.99901953e+00,  1.00000000e+00, 1.00000000e+00, -1.97638907e+00,  9.76726728e-01},
          { 1.00000000e+00, -1.99931660e+00,  1.00000000e+00, 1.00000000e+00, -1.98230743e+00,  9.82601536e-01},
          { 1.00000000e+00, -1.99947217e+00,  1.00000000e+00, 1.00000000e+00, -1.98725122e+00,  9.87512612e-01},
          { 1.00000000e+00, -1.99955888e+00,  1.00000000e+00, 1.00000000e+00, -1.99139129e+00,  9.91629854e-01},
          { 1.00000000e+00, -1.99960633e+00,  1.00000000e+00, 1.00000000e+00, -1.99496619e+00,  9.95190499e-01},
          { 1.00000000e+00, -1.99962752e+00,  1.00000000e+00, 1.00000000e+00, -1.99821287e+00,  9.98430473e-01}}
}

sqlitegocatorConfig = {
  Range = {0,99999999999999},
  Fps = gocatorFps,
  Forever = false,
  --Source = "../../profiles/rawprofile_cv001_2023-08-30.db",
  Source = "../../profiles/rawprofile_cv211_2023-11-08.db",
  TypicalSpeed = 6.0,
  Sleep = false
}

dbscan = {
  InClusterRadius = 12,
  MinPoints = 12,
  ZMergeRadius = 10,
  XMergeRadius = 50,
  MaxClusters = 2
}

revolutionsensor = {
  Source = "length",
  TriggerDistance = 0,
  Bias = 0,
  Threshold = 0.05,
  Bidirectional = false,
  Skip = 0
}

measures = {
  Enable = false
}

profileConfigTranslate = {
  Width = 0,
  NaNPercentage = 1,
  ClipHeight = 24.0,
  ClampToZeroHeight = 10.0,
  PulleyEstimatorInit = -15.0,
  PulleyCircumference = 0,
  TypicalSpeed = 0,
  WidthN = 0,
  IIRFilter = iirfilter,
  RevolutionSensor = revolutionsensor,
  Measures = measures
}


function main(sendmsg)

  local align_pulleyTranslate = BlockingReaderWriterQueue()
  local luamain = BlockingReaderWriterQueue()



  local toCSV = teeMsg(
    filterMsgs(13,extractPartition(0,fileCSV("cv211L.csv"))),
    filterMsgs(13,extractPartition(2,fileCSV("cv211R.csv")))
  )

  local tee = teeMsg(
    luamain,
    toCSV
  )

  local thread_profile_pulley_translate = profile_pulley_translate(profileConfigTranslate,align_pulleyTranslate,tee)
  local align_profile = alignProfile(iirfilter.Skip,align_pulleyTranslate)
  local partition_profile = partitionProfile(dbscan,align_profile)
  local laser = sqlitegocator(sqlitegocatorConfig,partition_profile) 

  laser:Start(gocatorFps)

  unloop = false
  repeat
    local is_value,msg_id,data = wait_for(luamain)

    if is_value then
      if msg_id == 2 then break 
      elseif msg_id == 5 then break
      elseif msg_id == 12 then
        cadsLog(data)
      end
    end

    unloop = coroutine.yield(0)
  until unloop

  laser:Stop(true)

  join_threads({thread_profile_pulley_translate})
  
end

mainco = coroutine.create(main)

-- gocator_properties = 0
-- scan = 1
-- finished = 2
-- begin_sequence =3 
-- end_sequence = 4
-- complete_belt = 5
-- pulley_revolution_scan = 6 
-- stopped = 7
-- nothing = 8
-- select = 9
-- caas_msg = 10
-- measure = 11
-- error = 12
-- profile_partitioned = 13