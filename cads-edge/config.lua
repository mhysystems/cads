belt = {
  Installed = "2023-01-14T00:00:00Z",
  PulleyCover = 7.0,
  CordDiameter = 9.1,
  TopCover = 14.0,
  Width = 1600,
  Length = 12545200,
  Splices = 1
}

anomaly = {
  WindowLength = 3 * 1000, -- In mm
  BeltPartitionLength = 500 * 1000 -- In mm
}

fifo = BlockingReaderWriterQueue()
gocator = mk_gocator(fifo,true,false)
thread = process_profile(fifo)

gocator:Start()

luamain({thread})