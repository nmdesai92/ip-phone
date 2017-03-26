VOIP in RTOS

The Server and client are seperate executables and used Socket APIs to transfer data.
->Server starts first and waiting for incoming calls.
->Client takes Server's IP Address and port number as arguments and connects with the server.
->Client records the incoming voice and send it through socket.
->Server receives incoming data and plays it.
->call disconnected as soon as client stops it.Socket is closed.
