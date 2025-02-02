package ec.edu.ups.momentos;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
// Importa el AssetManager
import android.content.res.AssetManager;

public class MainActivity extends AppCompatActivity {
    static {
        System.loadLibrary("native-lib");
    }

    private DrawView drawView;
    private TextView textView;

    // Actualiza la firma para recibir AssetManager
    private native String procesarDibujo(Bitmap bitmap, AssetManager assetManager);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        drawView = findViewById(R.id.drawView);
        textView = findViewById(R.id.textView);
        Button btnClasificar = findViewById(R.id.btnClasificar);
        Button btnLimpiar = findViewById(R.id.btnLimpiar);

        btnClasificar.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Bitmap bitmap = drawView.getBitmap();
                if (bitmap != null) {
                    // Pasar el AssetManager obtenido desde getAssets()
                    String classification = procesarDibujo(bitmap, getAssets());
                    textView.setText("Clasificación: " + classification);
                } else {
                    textView.setText("Error: No se pudo obtener el dibujo.");
                }
            }
        });

        btnLimpiar.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                drawView.clear();
                textView.setText("Dibuja una figura");
            }
        });
    }
}
