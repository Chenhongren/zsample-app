# Mars 5 Project Information

## Hardware Design
```
                        +-------------+
                        |MARS 5       |    +------------+
                        |       (i3c0)<----> ST LPS22DF |
       +------------+   |             |    +------------+
       | MXIC FLASH <--->(spi0 cs0)   |    +-----------------+   +------------+
       | mx25v1635f |   |             |    |TODO    (spi cs1)<---> MXIC FLASH |
       +------------+   |       (i3c1)<---->(i3c3)           |   | mx25l1006e |
                        +-------------+    +-----------------+   +------------+
```

## Software Design

