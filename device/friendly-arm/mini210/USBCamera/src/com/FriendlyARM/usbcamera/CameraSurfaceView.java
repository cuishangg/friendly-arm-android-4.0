package com.FriendlyARM.usbcamera;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.Paint;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;
import android.view.MotionEvent;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Context;
import com.FriendlyARM.usbcamera.OnCameraSurfaceViewEventListener;

public class CameraSurfaceView extends SurfaceView implements SurfaceHolder.Callback {

    private OnCameraSurfaceViewEventListener mListener;

    public CameraSurfaceView(Context context, AttributeSet attrs){
        super(context,attrs);
    }


    public CameraSurfaceView(Context context) {
        super(context);
    }

    public void setEventListener(OnCameraSurfaceViewEventListener eventListener) {
        mListener=eventListener;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width,int height) {}

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        int action = event.getAction();
        switch(action){
            case MotionEvent.ACTION_DOWN:
                Log.i("Camera", "TakePicture");
                if(mListener!=null)
                    mListener.onTouchEvent();
                break;
            default:
        }
        return true; //processed
    }

}
