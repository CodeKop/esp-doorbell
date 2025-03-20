# Esp Doorbell Software (Server Side)

## Routes
The following are the routes available from the server side of the software:

* ```register_subscriber```<br/>
Type: ```POST```<br/>
Return: ```subscriber_id?```<br/>
---<br/>
The register route will allow any devices to subscribe to the doorbell publisher, via its mac address and IP address. Any device that subscribed will be alerted when the doorbell is pressed.<br/>
_This feature could be added on by making the doorbell also report to the subscribed devices when it detects motion from the video stream. (potentially a power heavy featuer, though - will have to track it)._

* ```load_video_stream```<br/>
Type: ```GET?```<br/>
---<br/>
I'm not exactly sure how this would work, yet. But this route will allow the user to directly access the video stream coming from the camera.

* ```system_state```<br/>
Type: ```GET```<br/>
---<br/>
Returns information about the current state of the device.
Maybe include things like:
    * Battery Percentage
    * Time of day,
    * Atmospheric conditions (temperature, pressure, humidity) - Will require an additional module.
    * Light level.

* ```device_settings```<br/>
Type: ```GET```<br/>
---<br/>
Loads the current settings on the device.<br/>
---<br/>
Type: ```PUT```<br/>
This will update the current settings on the device. 

* ```version```<br/>
Type: ```GET```<br/>
---<br/>
This will return the version details of the software that is currently on the Doorbell, along with any other information that may be useful.
