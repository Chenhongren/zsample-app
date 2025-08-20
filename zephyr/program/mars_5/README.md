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

### Supported commands
#### platform info

<table>
  <tr>
     <td >ID</td>
     <td >Lable</td>
  </tr>
  <tr>
     <td >0</td>
     <td >platform name</td>
  </tr>
  <tr>
     <td >1</td>
     <td >firmware version</td>
  </tr>
  <tr>
     <td >2</td>
     <td >soc information (as known as soc device id)</td>
  </tr>

</table>

``` shell
uart:~$ platform info # list all
uart:~$ platform info <id>
```
#### system event log

<table>
  <tr>
     <td >Record Type</td>
     <td >Record Type Code</td>
     <td >Event Type</td>
     <td >Event Type Code</td>
     <td >Event Data</td>
     <td >Assert</td>
  </tr>

  <tr>
     <td >system</td>
     <td >0</td>
     <td >boot</td>
     <td >0</td>
     <td >always 0</td>
     <td >Assertion means bootup</td>
  </tr>

  <tr>
    <td rowspan="3">storage</td>
    <td >1</td>
    <td >project_info</td>
    <td >0</td>
    <td >always 0</td>
    <td >Failed to store platform info if assertion</td>
  </tr>
  <tr>
    <td >1</td>
    <td >record_id_rotate</td>
    <td >1</td>
    <td >always 0</td>
    <td >The record id(max. 255) is rotated if assertion</td>
  </tr>
  <tr>
    <td >1</td>
    <td >event_log_clear</td>
    <td >2</td>
    <td >always 0</td>
    <td >The system event log is cleared if assertion. ex: execute "sel clear" command</td>
  </tr>



  <tr>
     <td >kscan</td>
     <td >2</td>
     <td >keycode_changed</td>
     <td >0</td>
     <td >[00 row col]</td>
     <td >The assertion represents a key press event, and the deassertion represents a key release event.</td>
  </tr>

</table>

``` shell
uart:~$ sel list # list all
uart:~$ sel list <record id>
uart:~$ sel add <record type> <event type> <event data> <assert>
uart:~$ sel clear # clear all
```
