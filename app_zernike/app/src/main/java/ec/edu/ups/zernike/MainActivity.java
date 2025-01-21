package ec.edu.ups.zernike;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;

import ec.edu.ups.zernike.databinding.ActivityMainBinding;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'zernike' library on application startup.
    static {
        System.loadLibrary("zernike");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
    }

    /**
     * A native method that is implemented by the 'zernike' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}