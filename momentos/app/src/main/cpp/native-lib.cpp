#include <jni.h>
#include <string>
#include <vector>
#include <cmath>
#include <fstream>
#include <sstream>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <cfloat>
#include <android/bitmap.h>
#include <android/log.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

#define LOG_TAG "native-lib"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// --------------------------------------------------------------------------
// Función: calcularDistancia
// Calcula la distancia Manhattan entre dos vectores.
double calcularDistancia(const vector<double>& a, const vector<double>& b) {
    double distancia = 0.0;
    for (size_t i = 0; i < a.size(); i++) {
        distancia += fabs(a[i] - b[i]);
    }
    return distancia;
}

// --------------------------------------------------------------------------
// Función: normalizar
// Normaliza un vector usando Z-score.
vector<double> normalizar(const vector<double>& vec) {
    double suma = 0.0;
    for (double v : vec) {
        suma += v;
    }
    double media = suma / vec.size();

    double varianza = 0.0;
    for (double v : vec) {
        varianza += pow(v - media, 2);
    }
    double desviacion = sqrt(varianza / vec.size());

    vector<double> normalizado;
    for (double v : vec) {
        // Evitar división por cero
        if (desviacion != 0)
            normalizado.push_back((v - media) / desviacion);
        else
            normalizado.push_back(0.0);
    }
    return normalizado;
}

// --------------------------------------------------------------------------
// Función: calcularMomentosHu
// Calcula los 7 momentos de Hu a partir de una imagen (se espera imagen ya preprocesada).
vector<double> calcularMomentosHu(const Mat& imagen) {
    Moments m = moments(imagen, true);
    double hu[7];
    HuMoments(m, hu);
    vector<double> momentos(hu, hu + 7);
    return momentos;
}

// --------------------------------------------------------------------------
// Función: transformarHu
// Aplica la transformación logarítmica a los momentos de Hu para mejorar su discriminación.
vector<double> transformarHu(const vector<double>& hu) {
    vector<double> huLog;
    for (double val : hu) {
        // Evitar log(0) y preservar el signo: si el valor es muy pequeño se considera 0.
        double trans = (fabs(val) > 1e-10) ? -copysign(log10(fabs(val)), val) : 0;
        huLog.push_back(trans);
    }
    return huLog;
}

// --------------------------------------------------------------------------
// Función: preprocesarImagen
// Binariza la imagen, aplica un cierre morfológico y rellena el contorno principal para obtener una figura sólida.
Mat preprocesarImagen(const Mat& img) {
    Mat gris, imgBinaria;
    cvtColor(img, gris, COLOR_BGR2GRAY);
    // Binarización: Si el píxel es mayor a 128 se convierte en negro, y si es menor, en blanco
    threshold(gris, imgBinaria, 128, 255, THRESH_BINARY_INV);

    // Cierre morfológico para eliminar pequeños huecos
    Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
    morphologyEx(imgBinaria, imgBinaria, MORPH_CLOSE, kernel);

    // Rellenar la figura: detectar contornos y rellenar el más grande
    vector<vector<Point>> contornos;
    findContours(imgBinaria.clone(), contornos, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);
    Mat relleno = Mat::zeros(imgBinaria.size(), imgBinaria.type());
    if (!contornos.empty()) {
        // Seleccionar el contorno con el área más grande (asumiendo que es la figura principal)
        double maxArea = 0;
        int idxMax = -1;
        for (size_t i = 0; i < contornos.size(); i++) {
            double area = contourArea(contornos[i]);
            if (area > maxArea) {
                maxArea = area;
                idxMax = static_cast<int>(i);
            }
        }
        if (idxMax >= 0) {
            drawContours(relleno, contornos, idxMax, Scalar(255), FILLED);
        }
    }
    return relleno;
}

// --------------------------------------------------------------------------
// Función: leerMomentosDesdeCSV
// Lee el CSV de momentos (almacenado en assets) y retorna un vector de pares: (nombre_clase, vector_de_momentos).
vector<pair<string, vector<double>>> leerMomentosDesdeCSV(AAssetManager* mgr, const string& filename) {
    vector<pair<string, vector<double>>> momentos;
    AAsset* asset = AAssetManager_open(mgr, filename.c_str(), AASSET_MODE_STREAMING);
    if (!asset) {
        LOGE("No se pudo abrir el asset %s", filename.c_str());
        return momentos;
    }
    // Leer el contenido del asset
    size_t fileLength = AAsset_getLength(asset);
    string fileContent;
    fileContent.resize(fileLength);
    AAsset_read(asset, &fileContent[0], fileLength);
    AAsset_close(asset);

    // Procesar el contenido CSV
    stringstream ss(fileContent);
    string linea;
    while(getline(ss, linea)) {
        stringstream ls(linea);
        string clase;
        getline(ls, clase, ',');
        vector<double> vec;
        string token;
        while(getline(ls, token, ',')) {
            try {
                vec.push_back(stod(token));
            } catch (...) {
                // Ignorar errores de conversión
            }
        }
        if (vec.size() == 7)
            momentos.push_back({clase, vec});
    }
    return momentos;
}

// --------------------------------------------------------------------------
// Función auxiliar: bitmapToMat
// Convierte un objeto Bitmap de Android a un cv::Mat (se asume formato ARGB_8888).
bool bitmapToMat(JNIEnv* env, jobject bitmap, Mat& mat) {
    AndroidBitmapInfo info;
    if (AndroidBitmap_getInfo(env, bitmap, &info) < 0) {
        LOGE("Error al obtener la información del Bitmap");
        return false;
    }
    if (info.format != ANDROID_BITMAP_FORMAT_RGBA_8888) {
        LOGE("El Bitmap no está en formato RGBA_8888");
        return false;
    }
    void* pixels = nullptr;
    if (AndroidBitmap_lockPixels(env, bitmap, &pixels) < 0) {
        LOGE("Error al bloquear los píxeles del Bitmap");
        return false;
    }
    // Crear un cv::Mat y clonar los datos
    mat = Mat(info.height, info.width, CV_8UC4, pixels).clone();
    AndroidBitmap_unlockPixels(env, bitmap);
    // Convertir de RGBA a BGR
    cvtColor(mat, mat, COLOR_RGBA2BGR);
    return true;
}

// --------------------------------------------------------------------------
// Función nativa: procesarDibujo
// Se invoca desde MainActivity para procesar el dibujo realizado en el DrawView.
extern "C"
JNIEXPORT jstring JNICALL
Java_ec_edu_ups_momentos_MainActivity_procesarDibujo(JNIEnv *env, jobject /* this */, jobject bitmap, jobject assetManager) {
    // 1. Convertir el Bitmap a cv::Mat
    Mat imgOriginal;
    if (!bitmapToMat(env, bitmap, imgOriginal)) {
        return env->NewStringUTF("Error al convertir el Bitmap");
    }

    // 2. Preprocesar la imagen: binarización, cierre morfológico y rellenado
    Mat imgPreprocesada = preprocesarImagen(imgOriginal);

    // 3. Calcular los momentos de Hu y aplicar la transformación logarítmica
    vector<double> momentosFigura = calcularMomentosHu(imgPreprocesada);
    vector<double> momentosFiguraTrans = transformarHu(momentosFigura);
    vector<double> momentosFiguraNorm = normalizar(momentosFiguraTrans);

    // 4. Cargar el dataset de momentos de Hu desde CSV (en assets)
    AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
    if (!mgr) {
        return env->NewStringUTF("Error: No se pudo obtener el AssetManager");
    }
    string assetFile = "momentos.csv"; // Asegúrate de que el CSV esté en app/src/main/assets/
    vector<pair<string, vector<double>>> momentosReferencia = leerMomentosDesdeCSV(mgr, assetFile);
    if (momentosReferencia.empty()){
        return env->NewStringUTF("Error: No se pudo cargar el dataset de momentos");
    }

    // 5. Aplicar la misma transformación y normalización a los momentos de referencia
    for (auto& par : momentosReferencia) {
        vector<double> huTrans = transformarHu(par.second);
        par.second = normalizar(huTrans);
    }

    // 6. Clasificar la imagen comparando los momentos usando distancia Manhattan
    double menorDistancia = DBL_MAX;
    string claseClasificada = "Desconocido";
    for (const auto& ref : momentosReferencia) {
        double distancia = calcularDistancia(momentosFiguraNorm, ref.second);
        LOGE("Distancia a %s: %f", ref.first.c_str(), distancia);
        if (distancia < menorDistancia) {
            menorDistancia = distancia;
            claseClasificada = ref.first;
        }
    }

    // 7. Retornar la clasificación obtenida
    return env->NewStringUTF(claseClasificada.c_str());
}
