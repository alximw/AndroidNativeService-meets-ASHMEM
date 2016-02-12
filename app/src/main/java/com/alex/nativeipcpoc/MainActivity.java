package com.alex.nativeipcpoc;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

public class MainActivity extends AppCompatActivity implements View.OnClickListener{
    Button b;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        b=(Button)findViewById(R.id.button);
        b.setOnClickListener(this);

        try{
            boolean result=NativeMethods.startService();
            if(result){
                Log.i("OK","Success");
            }else{
                throw new Exception("MEEEEEC");
            }

        }catch (Exception e){
            Log.e("ERROR","error in native call: "+e.getMessage());
        }
    }

    @Override
    public void onClick(View v) {

        try{
            byte[]array_fromJNI=NativeMethods.getBuffer();
            if(array_fromJNI!=null) {
               String a=new String(array_fromJNI);
                Log.i("DATA FROM ASHMEM ",a);
            }

        }catch (Exception e){
            Log.e("ERROR","error in native call: "+e.getMessage());
        }
    }
}
