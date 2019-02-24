package com.android.server;

import android.net.LocalSocket;
import android.net.LocalSocketAddress;

import android.os.Bundle;
import android.util.Log;
import android.content.Context;

/* for thread */
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

public final class UDiskDSocket {
	private Handler messageHandler;
	
    private LocalSocket statusSocket = null;
    private LocalSocket cmdSocket = null;
    private BufferedReader inStatus = null;
    private BufferedReader inCmd = null;
    private PrintWriter outCmd = null;
    
    private RecvCallBack recvCallBack = null;
    private static final String TAG = "UDiskDSocket";
    private String mSocketName = "udiskd";
    private static final boolean _debug = false;
    
    public interface  RecvCallBack {  
    	void responseIncoming(String response);  
    }
    
    UDiskDSocket(RecvCallBack _recvCallBack)
    {
    	recvCallBack = _recvCallBack;
        Looper looper = Looper.myLooper();
        messageHandler = new MessageHandler(looper);
        
        if (!connectToSocketForStatus()) {
        	if (_debug) {
                Log.e(TAG, "#### Error: Fail to connect udiskd socket!");
            }
        }
        if (!connectToSocketForCMD()) {
        	if (_debug) {
                Log.e(TAG, "#### Error: Fail to connect udiskd socket!");
            }
        }
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
    
    public boolean connectToSocketForStatus()
    {
    	if (_debug) {
    		Log.d(TAG, "connectToSocketForStatus");
    	}
    	
		try {
    	    LocalSocketAddress address= new LocalSocketAddress(mSocketName
    			,LocalSocketAddress.Namespace.RESERVED);
    	    
    	    statusSocket = new LocalSocket();
    	    statusSocket.connect(address);
			if (statusSocket.isConnected()) {
				if (_debug) {
					Log.d(TAG, "Connected!");
				}
				inStatus = new BufferedReader(new InputStreamReader(statusSocket.getInputStream()));
			} else {
				return false;
			}
		} catch (IOException e) {
			if (_debug) {
				Log.d(TAG, "IOException: " + e);
			}
			return false;
		}
		
		startHanleStatusThread();
		if (_debug) {
			Log.d(TAG, "connectToSocketForStatus Finish!");
		}
		
		return true;
    }
    
    public boolean connectToSocketForCMD()
    {
    	if (_debug) {
    		Log.d(TAG, "connectToSocketForCMD");
    	}
    	
		try {
    	    LocalSocketAddress address= new LocalSocketAddress(mSocketName
    			,LocalSocketAddress.Namespace.RESERVED);
    	    
    	    cmdSocket = new LocalSocket();
    	    cmdSocket.connect(address);
			if (cmdSocket.isConnected()) {
				if (_debug) {
					Log.d(TAG, "Connected!");
				}
				inCmd = new BufferedReader(new InputStreamReader(cmdSocket.getInputStream()));
				outCmd = new PrintWriter(cmdSocket.getOutputStream()); 
			} else {
				return false;
			}
		} catch (IOException e) {
			if (_debug) {
				Log.d(TAG, "IOException: " + e);
			}
			return false;
		}
		
		recvResponse();
		if (_debug) {
			Log.d(TAG, "connectToSocketForCMD Finish!");
		}
		
		return true;
    }
    
    private void startHanleStatusThread()
    {
        new Thread() {
            @Override
            public void run() {
            	for (;;) {
            	    String result = new String("@ERROR");
            	    String line = null;
				    try {
					    line = inStatus.readLine();
					
					
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
            }

        }.start();
    }
    
    public void recvResponse()
    {
        new Thread() {
            @Override
            public void run() {
            	    String result = new String("@ERROR");
            	    String line = null;
				    try {
					    line = inCmd.readLine();
					
					
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
    	
    	if (inCmd == null || outCmd == null || cmdSocket == null) {
    		if (_debug) {
    			Log.d(TAG, "error, null object.");
    		}
    		return false;
    	}
    	
    	request += new String("\n");
    	outCmd.write(request);
		if (outCmd.checkError()) {
			return false;
		}
		
		return true;
    }
}
