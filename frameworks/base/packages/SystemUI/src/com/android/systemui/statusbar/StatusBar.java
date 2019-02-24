/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.systemui.statusbar;

import com.android.systemui.statusbar.SeviceSocket;
import android.content.ComponentName;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import java.io.File;
import java.io.FileReader;
import java.util.Timer;
import java.util.TimerTask;
import android.os.Handler;
import android.os.Message;
import android.os.ServiceManager;
import android.app.Notification;
import android.app.PendingIntent;

import android.app.ActivityManager;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.graphics.PixelFormat;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.util.Slog;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.WindowManagerImpl;

import java.util.ArrayList;

import com.android.internal.statusbar.IStatusBarService;
import com.android.internal.statusbar.StatusBarIcon;
import com.android.internal.statusbar.StatusBarIconList;
import com.android.internal.statusbar.StatusBarNotification;

import com.android.systemui.SystemUI;
import com.android.systemui.R;

public abstract class StatusBar extends SystemUI implements CommandQueue.Callbacks {
    static final String TAG = "StatusBar";
    private static final boolean SPEW = false;

    protected CommandQueue mCommandQueue;
    protected IStatusBarService mBarService;

    //F/r/i/e/n/d/l/y/A/R/M
    private static SeviceSocket seviceSocket = null;
    private NotificationManager myNotiManager;

    // Up-call methods
    protected abstract View makeStatusBarView();
    protected abstract int getStatusBarGravity();
    public abstract int getStatusBarHeight();
    public abstract void animateCollapse();

    private DoNotDisturb mDoNotDisturb;

 
    //{{F-r-i-e-n-d-l-y-A-R-M
    private void setStatusIcon(int iconId, String text) {
        Intent notifyIntent=new Intent();
        notifyIntent.setComponent(new ComponentName("com.friendlyarm.net3gdialup", "com.friendlyarm.net3gdialup.ActivityMain"));
        notifyIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        PendingIntent appIntent = PendingIntent.getActivity(mContext, 0,
            notifyIntent, 0);
        Notification myNoti = new Notification();
        myNoti.icon = iconId;
        myNoti.tickerText = text;
        myNoti.defaults = Notification.DEFAULT_LIGHTS;
        myNoti.setLatestEventInfo(mContext, "3G Network Status",text, appIntent);
        myNotiManager.notify(0, myNoti);
    }

    private void removeStatusIcon() {
        myNotiManager.cancelAll();
    }

    private int lastNetworkStatus = -1;
    private boolean isConnectService = false;

    private final int CONNECT_TO_SERVICE_MSG = 100;
    private final int REQUEST_NETSTATUS_MSG = 101;

    private Timer timerToConnService = new Timer();
    private Timer timerToRequestStatus = new Timer();

    private Handler handler = new Handler() {
        public void handleMessage(Message msg) {
            switch (msg.what) {
            case CONNECT_TO_SERVICE_MSG:
                timerToConnService.cancel();
                seviceSocket.connectToService();
                break;
            case REQUEST_NETSTATUS_MSG:
                seviceSocket.sendRequest("REQUEST NETSTATUS\n");
                seviceSocket.recvResponse();
                break;
            }
            super.handleMessage(msg);
        }
    };

    private TimerTask taskConnectService = new TimerTask() {
        public void run() {
            Message message = new Message();
            message.what = CONNECT_TO_SERVICE_MSG;
            handler.sendMessage(message);
        }
    };

    private TimerTask taskRequestNetStatus = new TimerTask() {
        public void run() {
            Message message = new Message();
            message.what = REQUEST_NETSTATUS_MSG;
            handler.sendMessage(message);
        }
    };

    private void processNETStatusResponse(String response) {
        String[] results = response.split(" ");

        if (response.startsWith("RESPONSE CONNECT OK")) {
             seviceSocket.sendRequest("REQUEST 3GAUTOCONNECT GETSTATUS");
             seviceSocket.recvResponse();
        } else if (response.startsWith(new String("RESPONSE 3GAUTOCONNECT")) && results.length == 6) {
            if (Integer.parseInt(results[2]) == 1 && results[3].startsWith(new String("3GNET"))) {
                timerToRequestStatus.schedule(taskRequestNetStatus,1,3000);
            } else {
                seviceSocket.disconnect();
            }
        } else if (response.startsWith(new String("RESPONSE NETSTATUS"))
            && results.length >= 5) {
            if (results[2].startsWith(new String("DOWN"))) {
                if (lastNetworkStatus != 0) {
                    removeStatusIcon();
                }
                lastNetworkStatus = 0;
            } else if (results[2].startsWith(new String("UP"))
                && results.length == 8) {
                if (lastNetworkStatus != 1) {
                    setStatusIcon(com.android.internal.R.drawable.net3g, "Connected. (FriendlyARM-3G)");
                }
                lastNetworkStatus = 1;
            }
        }
    }
    //}}



   public void start() {
        // First set up our views and stuff.


       View sb = makeStatusBarView();

        //F/r/i/e/n/d/l/y/A/R/M
        myNotiManager = (NotificationManager) mContext.getSystemService(Context.NOTIFICATION_SERVICE);
        seviceSocket = new SeviceSocket(new SeviceSocket.RecvCallBack() {
            public void responseIncoming(String response) {
            processNETStatusResponse(response);
            }
            });


        // Connect in to the status bar manager service
        StatusBarIconList iconList = new StatusBarIconList();
        ArrayList<IBinder> notificationKeys = new ArrayList<IBinder>();
        ArrayList<StatusBarNotification> notifications = new ArrayList<StatusBarNotification>();
        mCommandQueue = new CommandQueue(this, iconList);
        mBarService = IStatusBarService.Stub.asInterface(
                ServiceManager.getService(Context.STATUS_BAR_SERVICE));
        int[] switches = new int[7];
        ArrayList<IBinder> binders = new ArrayList<IBinder>();
        try {
            mBarService.registerStatusBar(mCommandQueue, iconList, notificationKeys, notifications,
                    switches, binders);
        } catch (RemoteException ex) {
            // If the system process isn't there we're doomed anyway.
        }

        disable(switches[0]);
        setSystemUiVisibility(switches[1]);
        topAppWindowChanged(switches[2] != 0);
        // StatusBarManagerService has a back up of IME token and it's restored here.
        setImeWindowStatus(binders.get(0), switches[3], switches[4]);
        setHardKeyboardStatus(switches[5] != 0, switches[6] != 0);

        // Set up the initial icon state
        int N = iconList.size();
        int viewIndex = 0;
        for (int i=0; i<N; i++) {
            StatusBarIcon icon = iconList.getIcon(i);
            if (icon != null) {
                addIcon(iconList.getSlot(i), i, viewIndex, icon);
                viewIndex++;
            }
        }

        // Set up the initial notification state
        N = notificationKeys.size();
        if (N == notifications.size()) {
            for (int i=0; i<N; i++) {
                addNotification(notificationKeys.get(i), notifications.get(i));
            }
        } else {
            Log.wtf(TAG, "Notification list length mismatch: keys=" + N
                    + " notifications=" + notifications.size());
        }

        // Put up the view
        final int height = getStatusBarHeight();

        final WindowManager.LayoutParams lp = new WindowManager.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                height,
                WindowManager.LayoutParams.TYPE_STATUS_BAR,
                WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE
                    | WindowManager.LayoutParams.FLAG_TOUCHABLE_WHEN_WAKING
                    | WindowManager.LayoutParams.FLAG_SPLIT_TOUCH,
                // We use a pixel format of RGB565 for the status bar to save memory bandwidth and
                // to ensure that the layer can be handled by HWComposer.  On some devices the
                // HWComposer is unable to handle SW-rendered RGBX_8888 layers.
                PixelFormat.RGB_565);
        
        // the status bar should be in an overlay if possible
        final Display defaultDisplay 
            = ((WindowManager)mContext.getSystemService(Context.WINDOW_SERVICE))
                .getDefaultDisplay();

        // We explicitly leave FLAG_HARDWARE_ACCELERATED out of the flags.  The status bar occupies
        // very little screen real-estate and is updated fairly frequently.  By using CPU rendering
        // for the status bar, we prevent the GPU from having to wake up just to do these small
        // updates, which should help keep power consumption down.

        lp.gravity = getStatusBarGravity();
        lp.setTitle("StatusBar");
        lp.packageName = mContext.getPackageName();
        lp.windowAnimations = R.style.Animation_StatusBar;
        WindowManagerImpl.getDefault().addView(sb, lp);

        if (SPEW) {
            Slog.d(TAG, "Added status bar view: gravity=0x" + Integer.toHexString(lp.gravity) 
                   + " icons=" + iconList.size()
                   + " disabled=0x" + Integer.toHexString(switches[0])
                   + " lights=" + switches[1]
                   + " menu=" + switches[2]
                   + " imeButton=" + switches[3]
                   );
        }

        mDoNotDisturb = new DoNotDisturb(mContext);
    }

    protected View updateNotificationVetoButton(View row, StatusBarNotification n) {
        View vetoButton = row.findViewById(R.id.veto);
        if (n.isClearable()) {
            final String _pkg = n.pkg;
            final String _tag = n.tag;
            final int _id = n.id;
            vetoButton.setOnClickListener(new View.OnClickListener() {
                    public void onClick(View v) {
                        try {
                            mBarService.onNotificationClear(_pkg, _tag, _id);
                        } catch (RemoteException ex) {
                            // system process is dead if we're here.
                        }
                    }
                });
            vetoButton.setVisibility(View.VISIBLE);
        } else {
            vetoButton.setVisibility(View.GONE);
        }
        return vetoButton;
    }
}
