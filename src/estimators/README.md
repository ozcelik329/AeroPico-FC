# Estimators

Bu klasor complementary filter, barometrik altitude estimator ve `BaroVerticalKalman` gibi durum tahmini kodlari icin ayrilmistir.

`BaroVerticalKalman` tek aktif dikey Kalman implementasyonudur. Bu bir tam-durum EKF degildir; yalnizca irtifa ve dikey hiz icin 2-durumlu Kalman filtresidir.

Kurallar:

- Donanim suruculerine dogrudan baglanma.
- Girdi olarak sade veri yapilari kullan.
- Ilk hedef tam otonomi degil, test edilebilir ve guvenilir durum tahminidir.
- Her estimator icin native unit test eklenmeden ucus akisine baglanmamalidir.
