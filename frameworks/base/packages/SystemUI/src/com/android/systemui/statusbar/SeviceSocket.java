package com.android.systemui.statusbar;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import java.util.Timer;
import java.util.TimerTask;

import android.os.Bundle;
import android.util.Log;
import android.content.Context;

import android.os.Message;
import android.os.Handler;
import android.os.Looper;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.PrintWriter;
import java.io.InputStreamReader;
import java.io.FileInputStream;
import java.io.BufferedReader;
import java.util.ArrayList;   
import java.util.HashMap;
import java.io.IOException;

//// F#R#I#E#N#D#L#Y#A#R#M
public final class SeviceSocket {
    private Handler messageHandler;
    private LocalSocket socket = null;
    private BufferedReader in = null;
    private PrintWriter out = null;
    private RecvCallBack recvCallBack = null;
    private static final String TAG = "FrameBaseServiceSocket";
    private static final boolean _debug = false;

    public interface  RecvCallBack {  
        void responseIncoming(String response);  
    }

    SeviceSocket(RecvCallBack _recvCallBack)
    {
        recvCallBack = _recvCallBack;
        Looper looper = Looper.myLooper();
        messageHandler = new MessageHandler(looper);
    }

    private class MessageHandler extends Handler {
        public MessageHandler(Looper looper) {
            super(looper);
        }

        @Override
            public void handleMessage(Message msg) {
                if (recvCallBack != null) {
                    recvCallBack.responseIncoming((String) msg.obj);
                }
            }
    }

    public void disconnect()
    {
        try {
            in.close();
            out.close();
            socket.close();
        } catch (IOException e) {
        }
    }

    public boolean connectToService()
    {
        if (_debug) {
            Log.d(TAG, "connectToService");
        }

        try {
            LocalSocketAddress address= new LocalSocketAddress("fa-network-service"
                ,LocalSocketAddress.Namespace.RESERVED);

            socket = new LocalSocket();
            socket.connect(address);
            if (socket.isConnected()) {
                if (_debug) {
                    Log.d(TAG, "Connected!");
                }
                in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
                out = new PrintWriter(socket.getOutputStream()); 
            } else {
                return false;
            }
        } catch (IOException e) {
            if (_debug) {
                Log.d(TAG, "IOException: " + e);
            }
            return false;
        }

        if (_debug) {
            Log.d(TAG, "connectToService Finish!");
        }

        recvResponse();

        return true;
    }

    public void recvResponse()
    {
        new Thread() {
            @Override
                public void run() {
                    String result = new String("@ERROR");
                    String line = null;
                    try {
                        line = in.readLine();


                        if (line != null && !(line.equals(""))) {
                            result = line;
                        }

                        if (_debug) {
                            Log.d(TAG, "@@@@: [" + line + "], result: " + result);
                        }

                    } catch (IOException e) {
                        if (_debug) {
                            Log.d(TAG, "IOException: " + e);
                            Log.d(TAG, "send message back");
                        }

                        Message message = Message.obtain();
                        message.obj = result;
                        messageHandler.sendMessage(message);
                        return;
                    }

                    Message message = Message.obtain();
                    message.obj = result;
                    messageHandler.sendMessage(message);
                }

        }.start();
    }


    public boolean sendRequest(String request)
    {
        if (_debug) {
            Log.d(TAG, ">>>>  " + request);
        }

        if (in == null || out == null || socket == null) {
            if (_debug) {
                Log.d(TAG, "error, null object.");
            }
            return false;
        }

        request += new String("\n");
        out.write(request);
        if (out.checkError()) {
            return false;
        }

        return true;
    }
}
