package ec.edu.ups.momentos;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class DrawView extends View {

    private Paint paint;
    private Path path;
    private Bitmap bitmap;
    private Canvas canvas;

    // Constructor con Context y AttributeSet (necesario para inflar desde XML)
    public DrawView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    // Constructor solo con Context
    public DrawView(Context context) {
        super(context);
        init();
    }

    // Método común para inicializar la vista
    private void init() {
        paint = new Paint();
        paint.setColor(0xFF000000); // Color negro para el dibujo
        paint.setStrokeWidth(10f);
        paint.setStyle(Paint.Style.STROKE);
        path = new Path();
    }

    @Override
    protected void onSizeChanged(int w, int h, int oldw, int oldh) {
        super.onSizeChanged(w, h, oldw, oldh);
        // Crear un nuevo Bitmap y Canvas cuando cambia el tamaño de la vista
        bitmap = Bitmap.createBitmap(w, h, Bitmap.Config.ARGB_8888);
        canvas = new Canvas(bitmap);
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        // Dibujar el bitmap en el canvas
        if (bitmap != null) {
            canvas.drawBitmap(bitmap, 0, 0, paint);
        }
        // Dibujar el path actual
        canvas.drawPath(path, paint);
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                path.moveTo(x, y);
                break;
            case MotionEvent.ACTION_MOVE:
                path.lineTo(x, y);
                break;
            case MotionEvent.ACTION_UP:
                // Dibujar el path en el bitmap
                canvas.drawPath(path, paint);
                path.reset(); // Reiniciar el path después de dibujar
                break;
        }

        invalidate(); // Redibujar la vista
        return true;
    }

    // Método para obtener el Bitmap del dibujo
    public Bitmap getBitmap() {
        return bitmap;
    }

    // Método para limpiar el espacio de dibujo
    public void clear() {
        if (bitmap != null) {
            bitmap.eraseColor(0xFFFFFFFF); // Limpiar el bitmap (fondo blanco)
        }
        path.reset(); // Reiniciar el path
        invalidate(); // Redibujar la vista
    }
}