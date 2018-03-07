This is a small server to provide authentication for Apple fairplay.

Read src/lib/fairplay.c in my shairplay repositary for a sample client code.

The code here is provided for people to use the service at foxsen.lemote.com.
I don't want to provide the fairplay.o file needed to compile the server to 
avoid possible legal risks.

Foxsen, 2015/4/23

For the protocol to communicate with this server:

First byte: 0x1/0x2/0x3， for fp-setup phase1/fp-setup phase2/fpdecrypt respectively;
The second byte: total length of the packet(0x12,0xA6,0x4A); 
Rest of the bytes: come from the binary data field of related ios senders(the first two directly from http requests' binary data, the key is base64 decoded from param1 of "POST /stream" request before iso9; in ios9, it comes from ekey field of SETUP request, plist_get_data_val will get it).

Foxsen, 2016/5/26: 
    Upload the binary .o and decoded parts' source code since I think it is ok. So you can build and change the server code as you like:
        fairplay-core.o the core part of fairplay protocol, heavily obfuscated to avoid people get the keys of out it(they must be embedded in it).
        fairplay.h: prototypes of decoded parts
        fairplay.c: some functions of fairplay-core.o are decompiled by me into the source code. By reading it, you can see the framework of fairplay obfuscation. 
        remap_segment.h: a data file. The secret keys should be here after some transforms. I have decoded one of them(the original code do xor for each data access, I have nullified it).
        airplay.c: an example file to test the routines. Very simple, read it and you will understand the whole process.

I hope someone will do further research(the goal is to get out the keys or fully decompile the code) with this since I can hardly find more time for this. Because you can now experiment on the core part of fairplay logic even without an apple device.It will be much more convenient to hack around.

I am in fact not very familiar with encryption/decryption. So even now we can easily get all the data access traces(Apple use special heap and stack to hide data, but this help us to get the access,turn on the debug flag,you will get traces), I still cannot find the core point of aes/rsa codes, which should lead to access of keyes(Of course, they can be further transformed).

