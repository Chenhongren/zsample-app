# Mars 5 Function Test Information

## Hardware Design
- Function board: I3CM/I3CS
```
  +------------+    +------------+
  |I3CM        |    |        I3CS|
  |      (i3c0)<---->(i3c2)      |
  |            |    |            |
  |      (i3c1)<---->(i3c3)      |
  +------------+    +------------+
```
- Function board: SPI
```
  +------------+
  |SPI         |     +------------+
  |    +-------+  +--> MXIC FLASH |
  |    |spi0   |  |  | mx25v1635f |
  |    |       |  |  +------------+
  |    |  (cs0)<--+
  |    |  (cs1)<--+
  |    +-------+  |  +------------+
  |            |  +--> MXIC FLASH |
  +------------+     | mx25l1006e |
                     +------------+
```
