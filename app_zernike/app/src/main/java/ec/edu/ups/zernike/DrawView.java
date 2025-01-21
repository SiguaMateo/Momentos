package ec.edu.ups.zernike;

import android.content.Context;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.Path;
import android.util.AttributeSet;
import android.view.MotionEvent;
import android.view.View;

public class DrawView extends View {
    private Paint paint;
    private Path path;

    public DrawView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        paint = new Paint();
        paint.setColor(0xFF000000); // Color negro
        paint.setStyle(Paint.Style.STROKE); // Sólo contornos
        paint.setStrokeWidth(10f); // Ancho del pincel

        path = new Path();
    }

    @Override
    protected void onDraw(Canvas canvas) {
        super.onDraw(canvas);
        canvas.drawPath(path, paint); // Dibujar el camino trazado
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        float x = event.getX();
        float y = event.getY();

        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                path.moveTo(x, y); // Inicia el dibujo
                break;
            case MotionEvent.ACTION_MOVE:
                path.lineTo(x, y); // Dibuja líneas a medida que se mueve el dedo
                break;
        }
        invalidate(); // Redibuja la vista
        return true;
    }
}